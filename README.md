# DigiHz_EasyTransfer
<i>The missing serial library with ack?</i>

Send and recieve any serial data with or without ack.

I was missing any library that could handle ack, so i made my own and wanted to share it to you all.

You can send / recieve vars, arrays, and structs.

And you can send / recieve chunks to. Look at the example.

Tested on arduino pro mini (3.3v), nodeMCU, ESP32 and ESP32-cam.

An example of sending esp32-cam captured image to another esp32 is now added.

The examples have a lot of documentation, so i dont have to explain to much here.

Limits of the Library:Max 254 bytes can be sent / recieved at a time

The protcol is as follows (Packet Anatomy):
Header1, Header2, Identifier, RequestAck, SizeofPayload, Payload, Checksum.

Any questions or issues are most welcome.
