from .engine import AnalysisEngine, datetime, timedelta, pd, DT_HOUR


"""
交易日历统计
"""


class CalendarEvents(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = "",
                 f_period: int = 15,
                 threshold: float = 5.0,
                 ):
        """
        :param symbol:
        :param start_date:
        :param end_date:
        :param f_period: 统计未来周期, 默认15
        :param threshold: 过滤阈值, 默认5$
        """
        super(CalendarEvents, self).__init__(symbol, "M1", start_date, end_date)

        self.f_period = f_period
        self.threshold = threshold
        self.data = self.get_bar()

    @staticmethod
    def process_events():
        cal_data = pd.read_csv("m_data/calendar_data.csv", encoding="gbk")

        # 1. 正则表达式筛选 01:00-14:00 出现的事件, 需要被剔除(?:表示括号内不捕获)
        cal_data = cal_data[~cal_data["时间"].str.contains("(?:(?:0[1-9])|(?:1[0-4])):", regex=True)]

        id_remove = []     # 设置需要被剔除的事件ID列表
        # 2. 最近一年未发布数据的事件需要被剔除
        filter_time = cal_data.groupby("事件ID").last()
        filter_time["Y"] = filter_time["时间"].str[:4].astype("int32")
        filter_time = filter_time.query("Y < 2020")
        id_remove.extend(filter_time.index.to_list())

        # 3.指标数量少于15条的事件需要被剔除(平均每年不到一次)
        filter_num = cal_data.groupby("事件ID").size()
        filter_num = filter_num[filter_num.values<15]
        id_remove.extend(filter_num.index.to_list())

        # 4. 集合去重
        id_remove = set(id_remove)
        cal_data = cal_data[~cal_data["事件ID"].isin(id_remove)]

        # 5. 添加标准时间索引
        cal_data.index = pd.to_datetime(cal_data["时间"])
        return cal_data

    def process_fluctuate(self):
        """
        1. 获取突破价格的bar索引
        2. 获取从突破到未来一定周期的df
        3. 统计df结果
        :param price: 要计算突破的价格, 正数表示向上突破
        :return:
        """
        rates = self.data.copy()
        # 1. 获取 f_period 周期的最大值和最小值再上移对应未来
        rates["hhigh"] = rates["high"].rolling(self.f_period).max().shift(-self.f_period)
        rates["llow"] = rates["low"].rolling(self.f_period).min().shift(-self.f_period)
        rates["copen"] = rates["close"]
        rates["cclose"] = rates["close"].shift(-self.f_period)

        # 2. 计算相关指标
        rates["fluctuate"] = rates["hhigh"] - rates["llow"]
        rates["trend"] = (rates["cclose"] - rates["copen"]).abs()
        rates["shock"] = (rates["fluctuate"] - rates["trend"]) / rates["fluctuate"]
        rates["up_value"] = rates["cclose"] - rates["copen"]
        rates["up_prob"] = (rates["up_value"] > 0)
        rates = rates.dropna().reset_index(drop=True)

        # 2. 合并数据(使用服务器时间)
        cal_data = self.process_events()
        df_fluctuate = cal_data.merge(rates, left_index=True, right_on="datetime_")
        # 3. 求波动均值并过滤
        gr_fluctuate = df_fluctuate.groupby("事件ID")["fluctuate"].mean()
        gr_fluctuate = gr_fluctuate[gr_fluctuate.values > self.threshold]
        print("均值波动超过阈值的事件数量: ", gr_fluctuate.shape[0])

        # 4. 筛选出对应的数据(会存在事件同时发布情况)
        df_fluctuate = df_fluctuate[df_fluctuate["事件ID"].isin(gr_fluctuate.index)]
        df_fluctuate.sort_values(by=["国家名称", "时间", "事件ID"], inplace=True)

        # 5. 将数据按[国家名称, 事件ID, 重要性] 分组, 然后求和 time(时间戳). 同时发布的数据时间戳之和必相等
        # 5.1 只用最近几年的数据做判断
        df_fluctuate["year"] = df_fluctuate["datetime_"].map(lambda x: x.year)
        df_time = df_fluctuate[df_fluctuate["year"] > 2018].copy()
        df_time = df_time.groupby(["国家名称", "事件ID", "重要性"])["time"].sum().reset_index()
        # 5.2 按照国家和time分组, 并选择重要性最大的事件ID的索引
        id_time = df_time.groupby(["国家名称", "time"])["重要性"].idxmax()
        # 5.3 最終事件筛选完毕
        df_time = df_time.iloc[id_time].reset_index()

        # 6. 统计筛选出来的数据
        df_result = df_fluctuate[df_fluctuate["事件ID"].isin(df_time["事件ID"].to_list())]
        # 6.1 计算最终指标数据
        df_result = df_result.groupby(["国家名称", "事件ID", "事件名称", "重要性"]).agg(
            {
                "fluctuate": "mean",
                "trend": "mean",
                "shock": "mean",
                "up_value": "mean",
                "up_prob": "mean",
                "close": "count",
            }).reset_index().rename({"close": "count"}, axis=1)

        # 7. 筛选出最近需要关注的时间
        # 7.1 获取未出现的事件, 用服务器时间
        cal_fut_data = cal_data[cal_data.index > datetime.now()-timedelta(hours=DT_HOUR)]
        # 7.2 按事件ID分组并取第一条数据(即即将公布事件)
        cal_fut_data = cal_fut_data.groupby("事件ID")["时间"].first().reset_index()
        # 7.3 将时间合并至处理后的数据
        df_result = df_result.merge(cal_fut_data, on="事件ID")
        # 7.4 为方便查阅, 将事件按时间排序
        df_result.sort_values(by="时间", inplace=True)
        return df_result

    def run(self):
        df_result = self.process_fluctuate()

        # 将涨跌P结果映射到[-1, 1]之间
        df_result["up_prob"] = (df_result["up_prob"] - 0.5) * 2
        # 设置精度
        df_result = df_result.round(
            {
                "fluctuate": self.digits,
                "trend": self.digits,
                "shock": 2,
                "up_value": self.digits,
                "up_prob": 2,
                "count": 0
            }
        )

        return df_result


if __name__ == '__main__':
    symbol = "XAUUSD"
    start_date = "20100101"
    end_date = (datetime.now()+timedelta(days=2)).date().isoformat().replace("-", "")

    ce = CalendarEvents(symbol, start_date, end_date, threshold=4.0, f_period=15)
    ce.run()
