**Is TA-Lib maintained?**

Yes.

*The Good News*

Many projects depending on TA-Lib are actively maintained.

The C/C++ source code have remained in-use for >20 years.

A large portion of TA-Lib is stable. Therefore, there is no further change needed for most TA functions.

*The Bad News*

Contributions to produce new TA functions are very rare.

Implementing speed efficient TA functions in C is harder compare to high-level languages provided by commercial software/website (e.g. tradingview, tradestation etc...).

Releases went stale between 2014 and 2024, one "excuse" is the packaging process was outdated and time-consuming.

*Ongoing Initiatives*

In 2024, work started to automate packaging with Github actions. Goal is to have new maintainers be able to trig a release at the "push of a button".

There is also work-in-progress to add a native Rust version.

Most importantly, there is intent to make the ta-lib-python installation easier.


**How to get support?**

Various ways:

- Have a friendly conversation with Mario Fortier (a.k.a Mhax) on discord:
![](https://dcbadge.limes.pink/api/server/Erb6SwsVbH)

- Open a [Github Issue](https://github.com/TA-Lib/ta-lib/issues)

- Check these communities:
    * [https://github.com/ta-lib/ta-lib-python](https://github.com/ta-lib/ta-lib-python)
    * [https://github.com/twopirllc/pandas-ta](https://github.com/twopirllc/pandas-ta)
