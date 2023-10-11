**Is TA-Lib maintained?**

Yes and No (depending of your perspective).

*The Good News*

Some projects depending on TA-Lib are still actively maintain.

The C/C++ source code have remained in-use for >20 years.

The C/C++ portion of TA-Lib is stable. Therefore, there is not much maintenance needed anymore.

*The Bad News*

The big **blocker** for applying a few minor fix to the code is the lack of resources (and interest) to maintain C/C++/binaries packaging for various OS/platforms. Also, the developer admit being afraid to touch it and have to suddenly support multiple platforms issues/limitation...

*The Plan*

Revive TA-Lib as a Rust package with Python bindings (using PyO3 Maturin).
These two languages have mature tooling for packaging and portable distribution.

There is hope that this will be easier to maintain by the community on long term.

Contact mario4tier on github if interested.

**Best place for support?**

Try these communities:

* [https://github.com/ta-lib/ta-lib-python](https://github.com/ta-lib/ta-lib-python)
* [https://github.com/twopirllc/pandas-ta](https://github.com/twopirllc/pandas-ta)
