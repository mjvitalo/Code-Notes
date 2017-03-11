# Standard Components of a distributed object/event/RPC solution

Component | Description
------------------------ | ---------------------
Protocol Message Format | Commands and responses, events. How methods to be invoked are identified, how event type are identified.
Data representation | Integer, pointer representations. Google Protocol buffers, GIOP/IIOP, msgpack, JASON
Message Framing Protocol | When using a bytestream you need an application level message framing to delimit messages in the stream. SCSI uses several: ISCSI, FC, SCSI bus.
Addressing | Where an object or service is located. Maybe an IP address and port. Corba Location (CorbaLoc) refers to a stringified object reference for a CORBA object that looks similar to a URL.
Communication Transport | SCTP, TCP, Fibre Channel FC-4 services. Encapsulates the transport so that errors can be differentiated from protocol errors. Strategized for performance and reliability.
Connection establishment | How do I locate and connect to services?
Physical Transport | Ethernet, FC-1, InfiniBand, LifeLink
