# Xanadu.Json

*A cross-platform Xanadu family of Json processing library*


## Explain

An Xanadu family of JSON libraries based on CJsonObject.


## Note

If you want to commit any changes, write the code in the following format.

set encoding=utf-8

set tabstop=8

set shiftwidth=8


## Comment format

~~~shell
/// <summary>
/// Overview of current call
/// </summary>
/// <param name="_ParameterName">Parameter description</param>
/// <returns>Return to description</returns>
/// <examples>Instructions</examples>
~~~


## Installation / Getting started

### Ubuntu Linux

First install all required dependencies and build tools:
```shell
sudo apt install XanaduRuntime XanaduCore
```

Then clone the actual project repository:
```shell
git clone https://github.com/XanaduAPI/XanaduJson.git
svn checkout https://github.com/XanaduAPI/XanaduJson/trunk
cd XanaduJson
```

Now you can build and install it:
```shell
cmake .
make
sudo make install
```

## License

This library is licensed under the [GNU Lesser General Public License v2.1](https://www.gnu.org/licenses/lgpl-2.1.en.html),
also included in the repository in the `COPYING` file.

README Updated on: 2021-01-14
