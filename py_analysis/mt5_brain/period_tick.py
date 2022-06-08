from .engine import AnalysisEngine, datetime


class PeriodTick(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = ""):
        super(PeriodTick, self).__init__(symbol, "", start_date, end_date)

        self.data = self.get_tick()

    def process_data(self, plot=False):
        rates = self.data.copy()
        rates["hour"] = rates["datetime"].map(lambda x: x.hour)
        rates["spread"] = rates["ask"] - rates["bid"]
        rates["temp"] = rates["spread"]

        result = rates.groupby("hour")["temp"].mean()
        if plot:
            self.plot_bar(result, title="点差")

        return result

    def run(self):
        result = self.process_data()
        return result


if __name__ == '__main__':
    symbol = "XAUUSD"
    start_date = "20220128"
    end_date = datetime.now().date().isoformat().replace("-", "")

    ques1 = PeriodTick(symbol, start_date, end_date)
    result = ques1.run()

    print(result)
