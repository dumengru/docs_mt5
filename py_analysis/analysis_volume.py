from mt5_brain.engine import AnalysisEngine, datetime


class PeriodTick(AnalysisEngine):
    def __init__(self,
                 symbol: str = "XAUUSD",
                 start_date: str = "",
                 end_date: str = ""):
        super(PeriodTick, self).__init__(symbol, "", start_date, end_date)

        self.data = self.get_tick()

    def process_data(self, plot=False):
        rates = self.data

        return rates

    def run(self):
        result = self.process_data()
        return result


if __name__ == '__main__':
    symbol = "FUTMGCJUN22"
    start_date = "20220421"
    end_date = datetime.now().date().isoformat().replace("-", "")

    ques1 = PeriodTick(symbol, start_date, end_date)
    result = ques1.run()

    result.to_csv("temp_anay.csv")
    print(result.head())
