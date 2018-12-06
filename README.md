
| CircleCI | VSTS |
| -------- | ---- |
| [![CircleCI branch](https://img.shields.io/circleci/project/github/emily33901/doghook/master.svg)](https://circleci.com/gh/emily33901/doghook) | [![Visual Studio Team services](https://img.shields.io/vso/build/f1ssi0n/a9fe704a-46e4-44b2-b9ed-1ab56526f533/2.svg)](https://f1ssi0n.visualstudio.com/doghook) |

# Doghook

## Making code changes

All code changes need to be made added to a new branch that follows the formula `<username>/<subject>` e.g. `marc3842h/circleci-artifacts`. Once you are done with your code changes make a pull request on github, add @josh33901 or @marc3842h as a reviewer. The pull request will make sure that it compiles on both platforms before it is allowed to be merged into the codebase.

Code should be clang-formatted before a pull request is made (otherwise it is a valid reason to decline a pull request).

## C++ guidelines

* Code should be formatted with the .clang-format in the root of the project (maybe make this a git hook??)
* Use almost always auto:
    
    This means using auto where the type is already observable. Examples are: 
    ```cpp
    auto v = *reinterpret_cast<LongAndAnnoyingType **>(vraw);
    auto f(int a, int b) {
        return a + b;
    }
    ```
    For functions that have a seperate declaration and definition do not use auto.
* Types are UpperCamelCase :camel:, whilst everything else is lower_snek_case :snake:. Do not use hungarian notation or `m_` for members or any similar prefix suffix system (`_t` for types).
* We are using C++ which means you dont need to do stupid C style struct typedef tricks. Remove these.


### Copy-pasted code blocks should be updated to match these guidelines to preserve consistency across the codebase!
