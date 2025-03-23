# Getting started

During the first startup, make active define in general.h

```cpp
#define FIRST_START_UP (1)
```

Then compile and upload file to the M5StickC.

After the first start up, define can be set to 0. No need to redefine it later.

```cpp
#define FIRST_START_UP (0)
```

In case of NTP server usage for the precise time usage, appropriate define should be set.

```cpp
#define USE_NTP_TIME   (1)
```

In case of NTP server usage, secret.h file should be defined. As a reference, "secret_example.h" can be taken
