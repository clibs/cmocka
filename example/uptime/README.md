The uptime mock example
=======================

This is a very simple example to explain the mocking feature of cmocka. It
implement the 'uptime' unix command in a very simple way to demonstrate how to
test the time calculation on a system which e.g. doesn't provide /proc/uptime.

This could be a jailed build environment for example!

Source files
------------

* *proc_uptime.c*: This implements the `uptime()` function reading and parsing
  the /proc/uptime file.
* *uptime.c*: This is the actual uptime implementation, it calls
  `calc_uptime()` to get a human readable string representation of the uptime.
  This function calls `uptime()` from proc_uptime.c.
* *test_uptime.c*: This is the test with the mocking function for uptime().

Linking magic
-------------

The test is linked using:

    ld --wrap=uptime

This replaces the orginal `uptime()` function which reads from `/proc/uptime`
with the mock function we implemented for testing `calc_uptime()`.

The mock function we implemented has a special name. It is called
`__wrap_uptime()`. All the symbols you want to mock (or replace) need to start
with the prefix `__wrap_`. So `ld --wrap=uptime` will rename the orignal
`uptime()` function to `__real_uptime()`. This means you can still reach the
original function using that name and call it e.g. from the wrap function.
The symbol `uptime` will be bound to `__wrap_uptime`.

You can find more details in the manpage: `man ld`

The uptime test
---------------

The code should be easy to understand. If you have a hard time following, there
are two ways to understand how things work.

You can find out details about symbol binding using:

    LD_DEBUG=symbols ./example/uptime/uptime
    LD_DEBUG=symbols ./example/uptime/test_uptime

You can also use a debugger to step through the code!


Have fun!
