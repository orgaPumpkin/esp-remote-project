# IR message
An IR message is any sequence of infrared pulses that the remote sends to the device.

# Message Types
## Constant message
A constant message is a message that doesn't contain any data.

For example, in an ac a constant message will be a `off` message,
as it doesn't need to pass any data, just the action of turning off.

There are two types of constant messages, one is a regular constant such as the previous example,
and the other one is a toggle constant.

A toggle is a message that flips the state of the device.
I.e., if you send the message twice, the device will return to its original state.

## Data message
A data message is a message that carries all the data to the device.

For example, in an ac device, a data message will contain the temperature, mode, fan speed, etc.

### Data message field
A data message is divided into fields.
The fields are sections of data in the message, each one responsible for a single attribute of the device.
The `temperature` in the above example is a field.

Each field has its own list of options, which are the possible values of the field.
For example, 26 degrees is an option of the `temperature` field.

### Base data message
A base data message is an example message
on which the esp-remote will construct the data message with the user's values.

The base data message needs to be a general data message,
it doesn't matter what its values are, just that it's not a constant message.
