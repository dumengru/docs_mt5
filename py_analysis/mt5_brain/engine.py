from MetaTrader5 import initialize, last_error, copy_rates_range, copy_ticks_range
from MetaTrader5 import (
    TIMEFRAME_M1,
    TIMEFRAME_M5,
    TIMEFRAME_M15,
    TIMEFRAME_M30,
    TIMEFRAME_H1,
    TIMEFRAME_H2,
    TIMEFRAME_H4,
    TIMEFRAME_D1,
    COPY_TICKS_ALL,
)
from datetime import datetime, timedelta
from pytz import timezone
import pandas as pd
import plotly.express as px
from plotly import offline
from time import sleep


pd.set_option("display.max_columns", 2000)
pd.set_option("display.width", 2000)

TIMEZONE = "Etc/UTC"        # 获取数据采用的时区    "Asia/Shanghai"      # "Etc/UTC"
DT_HOUR = 8                 # 本地时间 - 服务器时间差
if not initialize():
    print("initialize() failed, error code =", last_error())
    quit()


class AnalysisEngine:
    """
    分析问题三步走
    1. 获取数据
    2. 处理数据
    3. 绘制结果
    """

    def __init__(self,
                 symbol: str = "XAUUSD.c",
                 period: str = "",
                 start_date: str = "",
                 end_date: str = "",
                 digits: int = 2,
                 ):
        self.symbol = symbol

        # 方便书写period
        if period.lower() == "m1":
            self.period = TIMEFRAME_M1
        elif period.lower() == "m5":
            self.period = TIMEFRAME_M5
        elif period.lower() == "m15":
            self.period = TIMEFRAME_M15
        elif period.lower() == "m30":
            self.period = TIMEFRAME_M30
        elif period.lower() == "h1":
            self.period = TIMEFRAME_H1
        elif period.lower() == "h2":
            self.period = TIMEFRAME_H2
        elif period.lower() == "h4":
            self.period = TIMEFRAME_H4
        elif period.lower() == "d1":
            self.period = TIMEFRAME_D1
        else:
            self.period = period

        assert start_date != "" and end_date != "", "数据起始时间和结束时间有误"
        self.start_date = start_date
        self.end_date = end_date
        self.digits = digits

    def get_bar(self):
        """
        从MT5获取K线高开低收价
        :return: 获取数据结果
        """
        date_from = self.start_date
        date_to = self.end_date
        date_from = datetime(int(date_from[:4]), int(date_from[4:6]), int(date_from[6:]), tzinfo=timezone(TIMEZONE))
        date_to = datetime(int(date_to[:4]), int(date_to[4:6]), int(date_to[6:]), tzinfo=timezone(TIMEZONE))

        rates = copy_rates_range(self.symbol, self.period, date_from, date_to)
        rates = pd.DataFrame(rates)
        assert not rates.empty, "未获取到数据"
        # 1. 时间戳转时间
        rates["datetime_"] = pd.to_datetime(rates["time"], unit="s")
        # 2. 服务器时间转上海时间
        rates["datetime"] = rates["datetime_"].map(lambda x: x+timedelta(hours=DT_HOUR))

        return rates

    def get_tick(self):
        """
        从MT5获取Tick数据
        :return: 返回数据结果
        """
        date_from = self.start_date
        date_to = self.end_date
        date_from = datetime(int(date_from[:4]), int(date_from[4:6]), int(date_from[6:]), tzinfo=timezone(TIMEZONE))
        date_to = datetime(int(date_to[:4]), int(date_to[4:6]), int(date_to[6:]), tzinfo=timezone(TIMEZONE))

        rates = copy_ticks_range(self.symbol, date_from, date_to, COPY_TICKS_ALL)
        rates = pd.DataFrame(rates)
        assert not rates.empty, "未获取到数据"
        # 1. 时间戳转时间
        rates["datetime_"] = pd.to_datetime(rates["time"], unit="s")
        # 2. 服务器时间转上海时间
        rates["datetime"] = rates["datetime_"].map(lambda x: x+timedelta(hours=DT_HOUR))

        return rates

    def process_data(self):
        ...

    def plot_bar(self, data: pd.Series, title: str = "", skip_h: tuple = ()):
        """
        绘图, skip_h 跳过小时范围
        :param data:
        :param title:
        :param skip_h: 是一个二元组, 表示小时范围
        """
        fig = px.bar(data, x=data.index, y=data.values, title=title)

        if skip_h:
            fig.update_xaxes(
                rangebreaks=[
                    dict(bounds=[4, 12], pattern="hour"),  # hide hours outside of 9am-5pm
                ]
            )

        # 图片保存到本地, 浏览器打开更快速
        offline.plot(fig)
        #
        sleep(0.5)

    def run(self):
        ...


if __name__ == '__main__':
    symbol = "XAUUSD"
    period = "H1"
    start_date = "20220301"
    end_date = datetime.now().date().isoformat().replace("-", "")

    main_engine = AnalysisEngine(symbol, period, start_date, end_date)
    main_engine.run()

