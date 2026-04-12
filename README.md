# WFX Python

## Description

A C++ [WFX Plugin](https://www.ghisler.com/plugins.htm) for [Total Commander](https://www.ghisler.com/) that:

* serves as native 'bridge' to a user defines Python WFX extension for Total Commander
* gives YOU the possibility to extend Total Commander with Python
* samples are:
  - AWS S3 WFX Plugin using boto3
* is implemented using [pybind11](https://github.com/pybind/pybind11)
* can be used as template to create other C++ WFX plugins

## Preparation

### Python embedded

#### Create tree-threaded python embedded

Using latest python free threaded, for example 3.14t, run

```cmd
python3.14t -m compileall -b path\to\pythoncore-3.14t-64\Lib\
```

and then zip all generated `*.pyc` files into an `python314t.zip` (with folder `__phello__` etc in zip root).

Also copy the DLL from `path\to\pythoncore-3.14t-64\python314t.dll`.

#### Alternatively use official python embedded

If Total Commander might call from multiple threads, it might slow/block multithreading, 
when using python versions with GIL.

### pybind11

Create a python virtual environment called venv using the desired python version.

```cmd
python -m venv venv
```

Activate this venv:

```cmd
venv\Scripts\activate
```
then your path should look like:

```cmd
(venv) C:\src\wfx_python>
```

Add pybind11 within the venv using pip

```cmd
(venv) C:\src\wfx_python>pip install pybind11
```
