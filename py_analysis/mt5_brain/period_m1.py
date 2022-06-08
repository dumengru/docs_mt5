from .engine import AnalysisEngine, datetime, timedelta, pd
from MetaTrader5 import symbol_info_tick


"""
PeriodM1: 计算突破后的统计数据(不包含突破那根K)
VolumeM1: 计算增仓或减仓突破

"fluctuate": 波动, 用突破后15周期 hhigh - llow
"trend": 趋势, (15周期后的close - 突破K的close).abs()
"shock": 震荡, (fluctuate - trend) / fluctuate
"up_value": 上涨均值, (15周期后的close - 突破K的close).mean()
"up_prob": 上涨概率, (上涨+1, 否则为0).mean(). 最后会做调整 (-0.5)*2, 将值映射到[-1, 1]之间
"count": 突破次数
"price": 数值为正表示向上突破, 否则向下突破
"last_time": 最后一次突破的时间
"""


class PeriodM1(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = "",
                 mid_price: float = 0.0,
                 step: float = 1.0,
                 f_period: int = 15):
        """

        :param symbol:
        :param start_date:
        :param end_date:
        :mid_price price: 如果为正表示向上突破, 否则向下突破
        :param step: 统计mid_price上下节点数据
        :param f_period: 统计未来周期走势, 默认15
        """
        super(PeriodM1, self).__init__(symbol, "M1", start_date, end_date)

        self.mid_price = mid_price
        self.step = step
        self.f_period = f_period

        self.data = self.get_bar()

    def process_break(self, price):
        """
        1. 获取突破价格的bar索引
        2. 获取从突破到未来一定周期的df
        3. 统计df结果
        :param price: 要计算突破的价格, 正数表示向上突破
        :return:
        """
        rates = self.data.copy()
        # 1. 获取 f_period 周期的最大值和最小值再上移对应未来(shift多移动1位, 表示不包含突破K本身)
        rates["hhigh"] = rates["high"].rolling(self.f_period).max().shift(-self.f_period-1)
        rates["llow"] = rates["low"].rolling(self.f_period).min().shift(-self.f_period-1)
        rates["copen"] = rates["close"]
        rates["cclose"] = rates["close"].shift(-self.f_period-1)

        # 2. 计算相关指标
        rates["fluctuate"] = rates["hhigh"] - rates["llow"]
        rates["trend"] = (rates["cclose"] - rates["copen"]).abs()
        rates["shock"] = (rates["fluctuate"] - rates["trend"]) / rates["fluctuate"]
        rates["up_value"] = rates["cclose"] - rates["copen"]
        rates["up_prob"] = (rates["up_value"] > 0)
        rates = rates.dropna().reset_index(drop=True)

        if price > 0:
            rates_query = rates.query(f"open < {price} & close > {price}")
        else:
            rates_query = rates.query(f"open > {-price} & close < {-price}")

        df_result = rates_query.agg(
            {
                "fluctuate": "mean",
                "trend": "mean",
                "shock": "mean",
                "up_value": "mean",
                "up_prob": "mean",
                "close": "count",
            }).rename(index={"close": "count"})
        df_result["price"] = price
        df_result["last_time"] = rates_query["datetime"].values[-1]

        return df_result

    def run(self, f_len=5):
        """
        1. 获取价格上下范围的点
        2. 统计上下范围点的突破结果
        """
        result = []
        for i in range(-f_len, f_len+1):
            if self.mid_price > 0:
                price = self.mid_price + self.step * i
            else:
                price = self.mid_price - self.step * i
            result.append(self.process_break(price))

        df_result = pd.DataFrame(result)
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
                "count": 0,
                "price": self.digits,
            }
        )

        return df_result


class VolumeM1(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = "",
                 mid_price: float = 0.0,
                 step: float = 1.0,
                 bars: int = 3,
                 f_period: int = 15,
                 ):
        """
        :param symbol:
        :param start_date:
        :param end_date:
        :param mid_price: 如果为正表示向上突破, 否则向下突破
        :param step: 统计mid_price上下节点数据
        :param bars: 统计增仓/减仓, 正数为增仓, 负数为减仓
        :param f_period: 统计未来周期, 默认15
        """
        super(VolumeM1, self).__init__(symbol, "M1", start_date, end_date)

        self.mid_price = mid_price
        self.step = step
        self.bars = bars
        self.f_period = f_period

        self.data = self.get_bar()

    def process_break(self, price):
        """
        1. 获取突破价格的bar索引
        2. 获取从突破到未来一定周期的df
        3. 统计df结果
        :param price: 要计算突破的价格, 正数表示向上突破
        :return:
        """
        rates = self.data.copy()
        # 1. 获取 f_period 周期的最大值和最小值再上移对应未来(shift多移动1位, 表示不包含突破K本身)
        rates["hhigh"] = rates["high"].rolling(self.f_period).max().shift(-self.f_period-1)
        rates["llow"] = rates["low"].rolling(self.f_period).min().shift(-self.f_period-1)
        rates["copen"] = rates["close"]
        rates["cclose"] = rates["close"].shift(-self.f_period-1)

        # 2. 计算相关指标
        rates["fluctuate"] = rates["hhigh"] - rates["llow"]
        rates["trend"] = (rates["cclose"] - rates["copen"]).abs()
        rates["shock"] = (rates["fluctuate"] - rates["trend"]) / rates["fluctuate"]
        rates["up_value"] = rates["cclose"] - rates["copen"]
        rates["up_prob"] = (rates["up_value"] > 0)

        # 3. 计算过去bars的最大/小volume
        rates["vol_max"] = rates["tick_volume"].rolling(abs(self.bars)).max()
        rates["vol_min"] = rates["tick_volume"].rolling(abs(self.bars)).min()
        rates = rates.dropna().reset_index(drop=True)

        # 1. 价格筛选: 突破
        if price > 0:
            price_query = rates.query(f"open < {price} & close > {price}")
        else:
            price_query = rates.query(f"open > {-price} & close < {-price}")
        # 2. 成交量筛选: 增仓/减仓
        if self.bars > 0:
            vol_query = price_query.query(f"tick_volume == vol_max")
        else:
            vol_query = price_query.query(f"tick_volume == vol_min")

        # 有可能为空
        if vol_query.empty:
            return vol_query

        rates_query = vol_query
        df_result = rates_query.agg(
            {
                "fluctuate": "mean",
                "trend": "mean",
                "shock": "mean",
                "up_value": "mean",
                "up_prob": "mean",
                "close": "count",
            }).rename(index={"close": "count"})
        df_result["price"] = price
        df_result["last_time"] = rates_query["datetime"].values[-1]

        return df_result

    def run(self, f_len=5):
        """
        1. 获取价格上下范围的点
        2. 统计上下范围点的突破结果
        """
        result = []
        for i in range(-f_len, f_len+1):
            if self.mid_price > 0:
                price = self.mid_price + self.step * i
            else:
                price = self.mid_price - self.step * i
            # 避免返回值为空
            temp_result = self.process_break(price)
            if not temp_result.empty:
                result.append(temp_result)

        df_result = pd.DataFrame(result)

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
                "count": 0,
                "price": self.digits,
            }
        )

        return df_result


def answer_m1(up_down: int = 1):
    symbol = "XAUUSD"
    start_date = "20220125"
    end_date = (datetime.now()+timedelta(days=2)).date().isoformat().replace("-", "")

    symbol_info_tick_dict = symbol_info_tick(symbol)._asdict()
    bid = int(symbol_info_tick_dict["bid"])
    price = bid if up_down > 0 else -bid

    step = 1.0
    f_period = 15

    ques1 = PeriodM1(symbol, start_date, end_date, price, step, f_period)
    ques1.run().to_csv("m_data/突破向上.csv", index=False)

    ques1 = PeriodM1(symbol, start_date, end_date, -price, step, f_period)
    ques1.run().to_csv("m_data/突破向下.csv", index=False)

    bars = 5
    ques2 = VolumeM1(symbol, start_date, end_date, price, step, bars, f_period)
    ques2.run().to_csv("m_data/突破向上增仓.csv", index=False)
    ques2 = VolumeM1(symbol, start_date, end_date, price, step, -bars, f_period)
    ques2.run().to_csv("m_data/突破向上减仓.csv", index=False)
    ques2 = VolumeM1(symbol, start_date, end_date, -price, step, bars, f_period)
    ques2.run().to_csv("m_data/突破向下增仓.csv", index=False)
    ques2 = VolumeM1(symbol, start_date, end_date, -price, step, -bars, f_period)
    ques2.run().to_csv("m_data/突破向下减仓.csv", index=False)


if __name__ == '__main__':
    answer_m1()
