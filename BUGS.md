# Bugs

## FIX

  * Formatted messages don't have all mandatory fields present

  * Heartbeat messages are not sent

  * Messages are not verified for correctness (e.g. CheckSum)

  * Messages are not processed in-order

  * Messages are not resent upon request nor are gaps filled

  * Encryption is not handled at all. See session level test Ref ID 17.
