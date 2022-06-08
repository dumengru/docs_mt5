from mt5_brain.engine import datetime, timedelta
from MetaTrader5 import symbol_info_tick
from mt5_brain import (
    PeriodM1,
    VolumeM1,
    PeriodD1,
    PeriodH1,
    TodayH1,
    PeriodTick,
    CalendarEvents
)


SYMBOL = "XAUUSD.c"


def answer_d1(days: int = 120):
    # 1. 计算起始结束时间
    start_date = (datetime.now() - timedelta(days=days)).date().isoformat().replace("-", "")
    end_date = (datetime.now() + timedelta(days=2)).date().isoformat().replace("-", "")
    # 2. 获取结果
    ques = PeriodD1(SYMBOL, start_date, end_date)
    return ques.run()


def answer_period_h1(days: int = 30):
    # 1. 计算起始结束时间
    start_date = (datetime.now() - timedelta(days=days)).date().isoformat().replace("-", "")
    end_date = (datetime.now() + timedelta(days=2)).date().isoformat().replace("-", "")
    # 2. 获取结果
    ques = PeriodH1(SYMBOL, start_date, end_date)
    return ques.run()


def answer_today_h1(days: int = 30):
    # 1. 计算起始结束时间
    start_date = (datetime.now() - timedelta(days=days)).date().isoformat().replace("-", "")
    end_date = (datetime.now() + timedelta(days=2)).date().isoformat().replace("-", "")
    # 2. 获取结果
    ques = TodayH1(SYMBOL, start_date, end_date)
    return ques.process_data(True)


def answer_m1(days: int = 30, up_down: int = 1, f_len=5):
    # 1. 设置起始结束时间
    start_date = (datetime.now() - timedelta(days=days)).date().isoformat().replace("-", "")
    end_date = (datetime.now() + timedelta(days=2)).date().isoformat().replace("-", "")
    # 2. 获取最新价(价格为正: 向上突破, 为负: 向下突破)
    symbol_info_tick_dict = symbol_info_tick(SYMBOL)._asdict()
    bid = int(symbol_info_tick_dict["bid"])
    mid_price = bid if up_down > 0 else -bid

    step = 1.0
    f_period = 15

    ques1 = PeriodM1(
        symbol=SYMBOL,
        start_date=start_date,
        end_date=end_date,
        mid_price=mid_price,
        step=step,
        f_period=f_period
    )
    return ques1.run(f_len)


def answer_vol_m1(days: int = 30, up_down: int = 1, bars: int = 3, f_len=5):
    # 1. 设置起始结束时间
    start_date = (datetime.now() - timedelta(days=days)).date().isoformat().replace("-", "")
    end_date = (datetime.now() + timedelta(days=2)).date().isoformat().replace("-", "")
    # 2. 获取最新价(价格为正: 向上突破, 为负: 向下突破)
    if up_down in [1, -1]:
        symbol_info_tick_dict = symbol_info_tick(SYMBOL)._asdict()
        bid = int(symbol_info_tick_dict["bid"])
        mid_price = bid if up_down > 0 else -bid
    else:
        mid_price = up_down

    step = 1.0
    f_period = 15

    ques1 = VolumeM1(
        symbol=SYMBOL,
        start_date=start_date,
        end_date=end_date,
        mid_price=mid_price,
        step=step,
        bars=bars,
        f_period=f_period
    )
    return ques1.run(f_len)


def answer_calendar(f_period=15, threshold=4.0):
    start_date = "20100101"
    end_date = (datetime.now()+timedelta(days=2)).date().isoformat().replace("-", "")

    ce = CalendarEvents(SYMBOL, start_date, end_date, f_period=f_period, threshold=threshold)
    return ce.run()


# 日线统计
d = answer_d1(days=120)
# 小时统计
h = answer_period_h1(days=30)
ht = answer_today_h1(days=30)
print(d)
print(h)
print(ht)

# 分钟统计
ans0 = answer_m1(days=30, up_down=1, f_len=1)
ans1 = answer_m1(days=30, up_down=-1, f_len=1)
print(ans0)
print(ans1)


# 保存数据到本地
# 大周期统计数据
answer_d1(days=120).to_csv("m_data/1D统计.csv")
answer_period_h1(days=30).to_csv("m_data/1H统计.csv")
answer_today_h1(days=30).to_csv("m_data/1HToday统计.csv", index=False)

# 价格突破数据
answer_m1(days=30, up_down=1, f_len=20).to_csv("m_data/突破向上.csv", index=False)
answer_m1(days=30, up_down=-1, f_len=20).to_csv("m_data/突破向下.csv", index=False)
answer_vol_m1(days=30, up_down=1, bars=5, f_len=20).to_csv("m_data/突破向上增仓.csv", index=False)
answer_vol_m1(days=30, up_down=1, bars=-5, f_len=20).to_csv("m_data/突破向上减仓.csv", index=False)
answer_vol_m1(days=30, up_down=-1, bars=5, f_len=20).to_csv("m_data/突破向下增仓.csv", index=False)
answer_vol_m1(days=30, up_down=-1, bars=-5, f_len=20).to_csv("m_data/突破向下减仓.csv", index=False)

# 交易日历数据(涉及日历都需要gbk格式)
answer_calendar(f_period=15, threshold=4.0).to_csv("m_data/日历统计_15_4.csv", index=False, encoding="gbk")
answer_calendar(f_period=15, threshold=5.0).to_csv("m_data/日历统计_15_5.csv", index=False, encoding="gbk")

