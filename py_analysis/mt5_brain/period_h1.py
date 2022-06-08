from .engine import AnalysisEngine, datetime, pd
from datetime import time
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from plotly import offline


"""
PeriodH1: 统计历史数据24小时情况
process_fluctuate: 波动
process_trend: 趋势
process_shock: 震荡
process_up_value: 涨幅
process_up_prob: 上涨概率. 最后会被映射到[-1, 1]

TodayH1: 统计今天的数据和历史数据的异常
"""


class PeriodH1(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = ""):
        super(PeriodH1, self).__init__(symbol, "H1", start_date, end_date)

        # 1. 获取数据
        self.data = self.get_bar()
        # 2. 添加小时(方便绘图)
        self.data["hour"] = self.data["datetime"].map(lambda x: time(hour=x.hour))
        # 3. 设置跳过的时间点范围
        self.skip_h = (4, 13)
        # 4. 需要跳过的时间点(无意义的小时)
        h_index = [time(hour=i) for i in range(*self.skip_h)]
        self.data = self.data[~self.data["hour"].isin(h_index)]
        # 5. 将索引替换为小时
        self.data.set_index(keys="hour", drop=True, inplace=True)

    def process_fluctuate(self, plot=False):
        """
        统计波动, h-l求均值
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = rates["high"] - rates["low"]
        result = rates.groupby("hour")["temp"].mean()

        if plot:
            self.plot_bar(result, title="波动", skip_h=self.skip_h)
        return result

    def process_trend(self, plot=False):
        """
        统计趋势, (c-o).abs()
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = (rates["close"] - rates["open"]).abs()
        result = rates.groupby("hour")["temp"].mean()
        if plot:
            self.plot_bar(result, title="趋势", skip_h=self.skip_h)
        return result

    def process_shock(self, plot=False):
        """
        统计震荡概率, bar影线比例
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = (rates["high"] - rates["low"] - (rates["close"] - rates["open"]).abs())/(rates["high"] - rates["low"])
        result = rates.groupby("hour")["temp"].mean()

        if plot:
            self.plot_bar(result, title="震荡", skip_h=self.skip_h)
        return result

    def process_up_value(self, plot=False):
        """
        统计上涨值, 负值表示下跌
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = rates["close"] - rates["open"]
        result = rates.groupby("hour")["temp"].mean()

        if plot:
            self.plot_bar(result, title="上涨值", skip_h=self.skip_h)
        return result

    def process_up_prob(self, plot=False):
        """
        统计上涨概率, 值域[-1, 1]
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = (rates["close"] - rates["open"] > 0)
        result = rates.groupby("hour")["temp"].sum() / rates.groupby("hour")["temp"].count()

        if plot:
            self.plot_bar(result, title="上涨概率", skip_h=self.skip_h)
        return result

    def run(self, plot: bool = False):
        fluctuate = self.process_fluctuate(plot)
        trend = self.process_trend(plot)
        shock = self.process_shock(plot)
        up_value = self.process_up_value(plot)
        up_prob = self.process_up_prob(plot)
        df_result = pd.DataFrame(
            {
                "fluctuate": fluctuate.values,
                "trend": trend.values,
                "shock": shock.values,
                "up_value": up_value.values,
                "up_prob": up_prob.values,
            },
            index=fluctuate.index
        )

        # 将涨跌P结果映射到[-1, 1]之间
        df_result["up_prob"] = (df_result["up_prob"] - 0.5) * 2
        # 设置精度
        df_result = df_result.round(
            {
                "fluctuate": self.digits,
                "trend": self.digits,
                "shock": 2,
                "up_value": self.digits,
                "up_prob": 2
            }
        )

        return df_result


class TodayH1(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = ""):
        super(TodayH1, self).__init__(symbol, "H1", start_date, end_date)

        # 1. 获取数据
        self.data = self.get_bar()
        # 2. 添加小时(方便绘图)
        self.data["hour"] = self.data["datetime"].map(lambda x: time(hour=x.hour))
        # 3. 设置跳过的时间点范围
        self.skip_h = (4, 7)
        # 4. 需要跳过的时间点(无意义的小时)
        h_index = [time(hour=i) for i in range(*self.skip_h)]
        self.data = self.data[~self.data["hour"].isin(h_index)]

    def process_data(self, plot=False):
        """
        统计波动, h-l求均值
        :param plot:
        :return:
        """
        # 5. 找到最后一个交易日的索引, 将数据分为历史数据和当日数据
        last_group = self.data.groupby("hour").last()
        time_index = last_group.loc[time(hour=7), "time"]
        his_data, today_data = self.data.query(f"time < {time_index}").copy(), self.data.query(f"time >= {time_index}").copy()

        his_data["fluctuate"] = his_data["high"] - his_data["low"]
        his_data["trend"] = (his_data["close"] - his_data["open"]).abs()

        his_result = his_data.groupby("hour").agg(
            {
                "fluctuate": ["mean", "max", "min"],
                "trend": ["mean", "max", "min"],
                "tick_volume": ["mean", "max", "min"],
            }).rename(index={"close": "count"})
        his_result.columns = ["_".join(i) for i in his_result.columns.to_flat_index()]

        today_data["fluctuate"] = today_data["high"] - today_data["low"]
        today_data["trend"] = (today_data["close"] - today_data["open"]).abs()
        today_result = today_data[["hour", "fluctuate", "trend", "tick_volume"]]
        df_result = today_result.merge(his_result, left_on="hour", right_index=True)

        if plot:
            self.plot_today(df_result)

        return df_result

    @staticmethod
    def plot_today(df_result):
        fig = make_subplots(
            rows=3, cols=1,
            shared_xaxes=True,
            vertical_spacing=0.03,
        )

        # 绘制fluctuate
        result = ["fluctuate", "trend", "tick_volume"]
        for index, name in enumerate(result):
            fig.add_trace(go.Bar(x=df_result["hour"], y=df_result[f"{name}_min"], name=f"{name}_min"), row=index+1, col=1)
            fig.add_trace(go.Bar(x=df_result["hour"], y=df_result[f"{name}_max"]-df_result[f"{name}_min"], name=f"{name}_max"), row=index+1, col=1)
            fig.add_trace(go.Scatter(x=df_result["hour"], y=df_result[f"{name}_mean"], mode="lines", name=f"{name}_mean"), row=index+1, col=1)
            fig.add_trace(go.Scatter(x=df_result["hour"], y=df_result[f"{name}"], mode="lines", name=f"{name}"), row=index+1, col=1)

        fig.update_layout(
            height=800,
            width=1200,
            title_text="Today",
            barmode='stack'
        )

        # 图片保存到本地, 浏览器打开更快速
        offline.plot(fig)

    def run(self, plot: bool = False):
        df_result = self.process_data()

        return df_result


def answer_h1():
    symbol = "XAUUSD"
    start_date = "20220301"
    end_date = datetime.now().date().isoformat().replace("-", "")

    ques = PeriodH1(symbol, start_date, end_date)
    return ques.run()


if __name__ == '__main__':
    answer_h1()