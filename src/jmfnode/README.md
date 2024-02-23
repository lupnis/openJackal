# openJackal Mirrors Fetching Node

### Descriptions

**jmfnode** carries the main syncing and reporting tasks. This part is the fundamental part of the whole openJackal Mirrorstation.

This part is constructed with capabilities of both downloading with multi-thread, and multi-tasks running. It also offers a more safe way to exit without losing tasks unfinished.

You can also use it as a downloader for other purposes.

------

### Architecture

The architecture of jmfnode is shown in the following figure:

<p align="center"><img src="../../res/jmfnode/arch_jmfnode.svg" alt="fig_arch" /></p>

The network traffic flow is shown in the following figure:

<p align="center"><img src="../../res/jmfnode/traffic_jmfnode.svg" alt="traffic_arch" /></p>

------

Documents about designs of database ~~are described in the root [README.md](../../README.md) file~~ haven't finished yet, stay tuned. (2024/02/23)

### Build and Run

<pre><strong>NOTICE:</strong> the program <strong>HAS NOT</strong> been verified on operating systems other than Windows. (upd on 2024/02/23)</pre>

#### Dependencies / Environments
The following dependencies / environments are required to build and run jmfnode:
+ dependencies:
    + **qt library**: basis of the program
    + **hiredis**: supporting communications with redis
+ environments:
    + **MySQL**: deployed on database server
    + **redis**: deployed on database server
    + **MySQL ODBC Connector**: supporting communications with MySQL

#### Preparations for Build

hiredis should be built in advance to guarantee the communication between jmfnode and redis.

use the cmake installed by qt to build the redis library, please build directly inside **src/3rdparty/hiredis/build**, or build and manually change the link in the jmfnode.pro:

```
unix|win32: LIBS += -L../3rdparty/hiredis/build -lhiredis # <- change to the path you build
INCLUDEPATH += ../3rdparty/hiredis # <- change to the path of hiredis source
DEPENDPATH += ../3rdparty/hiredis # <- change to the path of hiredis source
```

#### Build

On Windows, directly double click the **jmfnode.pro**, enter Qt Creator, auto configure and then press build to build the application.

------
