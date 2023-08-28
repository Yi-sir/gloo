# Error handling
Sophon defines two groups of exceptions for raising errors: recoverable errors and
assertions.

## Recoverable errors
Sophon throws exceptions for potentially recoverable errors. The most common
examples include IO errors, where a connected machine or network link is
in a bad state, but the local machine is operational. The caller, with
appropriate context, may be able to recover or reconfigure the machine pool.

For `::sophon::IoException` specifically, the caller should assume the
transport layer is in an unknown state and recreate the transport
[pairs](../sophon/transport/pair.h) or containing collective algorithm instance.

Exceptions are defined in [`error.h`](../sophon/common/error.h) and should
extend `::sophon::Exception`.

## Assertions
Sophon asserts unexpected errors and logical invariants instead of expecting
callers to handle them. `SOPHON_ENFORCE` macros are defined in
[`logging.h`](../sophon/common/logging.h)
