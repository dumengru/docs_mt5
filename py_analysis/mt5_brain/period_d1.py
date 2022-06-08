from .engine import AnalysisEngine, datetime, pd


"""
由于服务器和本地时间不一致, 因此统计Day采用服务器时间
"""

MAP_INDEX = {
    0: "Mon",
    1: "Tue",
    2: "Wed",
    3: "Thu",
    4: "Fri",
    5: "Sat",
    6: "Sun",
}


class PeriodD1(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = ""):
        super(PeriodD1, self).__init__(symbol, "D1", start_date, end_date)

        # 1. 获取数据
        self.data = self.get_bar()
        # 3. 用服务器时间获取周几数
        self.data["day"] = self.data["datetime_"].map(lambda x: x.weekday())

    def process_fluctuate(self, plot=False):
        """
        统计波动, h-l求均值
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = rates["high"] - rates["low"]
        result = rates.groupby("day")["temp"].mean()
        result.rename(index=MAP_INDEX, inplace=True)

        if plot:
            self.plot_bar(result, title="波动")
        return result

    def process_trend(self, plot=False):
        """
        统计趋势, (c-o).abs()
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = (rates["close"] - rates["open"]).abs()
        result = rates.groupby("day")["temp"].mean()
        result.rename(index=MAP_INDEX, inplace=True)

        if plot:
            self.plot_bar(result, title="趋势")
        return result

    def process_shock(self, plot=False):
        """
        统计震荡概率, bar影线比例
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = (rates["high"] - rates["low"] - (rates["close"] - rates["open"]).abs())/(rates["high"] - rates["low"])
        result = rates.groupby("day")["temp"].mean()
        result.rename(index=MAP_INDEX, inplace=True)

        if plot:
            self.plot_bar(result, title="震荡")
        return result

    def process_up_value(self, plot=False):
        """
        统计上涨值, 负值表示下跌
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = rates["close"] - rates["open"]
        result = rates.groupby("day")["temp"].mean()
        result.rename(index=MAP_INDEX, inplace=True)

        if plot:
            self.plot_bar(result, title="上涨值")
        return result

    def process_up_prob(self, plot=False):
        """
        统计上涨概率, 值域[-1, 1]
        :param plot:
        :return:
        """
        rates = self.data.copy()
        rates["temp"] = (rates["close"] - rates["open"] > 0)
        result = rates.groupby("day")["temp"].sum() / rates.groupby("day")["temp"].count()
        result.rename(index=MAP_INDEX, inplace=True)

        if plot:
            self.plot_bar(result, title="上涨概率")
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
            index=fluctuate.index,
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


def answer_d1():
    symbol = "XAUUSD"
    start_date = "20210401"
    end_date = datetime.now().date().isoformat().replace("-", "")

    ques = PeriodD1(symbol, start_date, end_date)
    return ques.run()


if __name__ == '__main__':
    answer_d1()