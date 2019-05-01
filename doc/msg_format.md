## TibMsg message format

```console
          Version        NameSize        Type Size
|- Magic ---|  |- MsgSize -|  |- Name ----|  |  |- Data ----|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|ce 13 aa 1f|01|00 00 00 0b|04|6e 61 6d 00|05|04|12 34 56 78|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|---- Header : 9 bytes ----|--- Field ----------------------|
```

* The first 4 bytes is a magic number, it is always 0xce13aa1f.

* The 4 bytes after the version is the size of the message, which does
not include the 9 byte header of the message.

* Field names have a maximum of 255 bytes, the null byte which terminates a C
  string is included in the size of the field name.  A zero size field name
  indicates the name is NULL.

* TibMsg field types:
  * 0 = none
  * 1 = message
  * 2 = string
  * 3 = opaque
  * 4 = boolean
  * 5 = int
  * 6 = unsigned int
  * 7 = real
  * 8 = array
  * 9 = partial
  * 10 = ip data (4 byte ip address or 2 byte port)

* The type + size is encoded several ways:
  * 0x00 -> 0x0f : lower 16 bits are the type of the field
  * 0x80 clear   : indicates the next byte is the size
  * 0x80 set     : indicates the next 4 bytes are the size
  * 0x40 set     : indicates that there is hint data

Examples of type + size:
  * 0x05 0x02 : type is int(5), size is 2.
  * 0x82 0x00 0x00 0x04 0x00 : type is string(2), size is 1024 (0x400).
  * 0x42 0x0c : type is string(2), size is 12, field has hint data.

* Hint data is necessary when describing array types, the partial offset,
and is also used to indicate whether a string is a date or a time.

Example of encoding a string with a date hint:

```console
 11 : field name size                0x42 : 0x40 = has hint
  | field name "HSTCLSDATE"           |     0x02 = string type
  V                                   V 12 : size of data
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|0B|48 53 54 43 4C 53 44 41 54 45 00|42 0C|32 35 20 41 50 52
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     H  S  T  C  L  S  D  A  T  E \0        2  5     A  P  R

                   0x06 : hint type unsigned int
                    | 0x2 : hint size = 2
                    V  V 
+--+--+--+--+--+--+--+--+--+--+
 20 31 39 39 34 00|06 02|01 02|
+--+--+--+--+--+--+--+--+--+--+
     1  9  9  4 \0       0x102 = 258  : hint value, which is the hint that
                                        indicates string is a Marketfeed date
```

Example of encoding partial data, which includes an offset:

```console
  8 : field name size      0x49 : 0x40 = has hint
  | field name "ROW64_1"     |    0x09 = partial type
  V                          V 4 : size        v- v- 6 : uint, 0x3c : offset 60
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|08|52 4F 57 36 34 5F 34 00|49 04|41 41 55 55|06 3C|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     R  O  W  6  4  _  1 \0        A  A  U  U 
```

This is a partial update message, patch AAUU at offset 60.  The size of the
hint type is used as the partial offset.


Example of encoding array data, which includes a type size.

```console
  6 : field name size  0x48 : 0x40 = has hint
  | field name "ARRAY"  |     0x08 = array type
  V                     V 6 : size             v- v- 5 : int, 2 : element size
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|06|41 52 52 41 59 00|48 06|00 01 00 02 00 03|05 02|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     A  R  R  A  Y \0      [  1,    2,    3  ]
```

This is a short int array with an element size of 2 and the array data has 3
elements: [ 1, 2, 3 ].

Hint integer values used with reals and strings:

  * 0 : real decimal no hint
  * 1 : real decimal fraction 1/2
  * 2 : real decimal fraction 1/4
  * 3 : real decimal fraction 1/8
  * 4 : real decimal fraction 1/16
  * 5 : real decimal fraction 1/32
  * 6 : real decimal fraction 1/64
  * 7 : real decimal fraction 1/128
  * 8 : real decimal fraction 1/256
  * 17 : real decimal precision 10<sup>-1</sup>
  * 18 : real decimal precision 10<sup>-2</sup>
  * 19 : real decimal precision 10<sup>-3</sup>
  * 20 : real decimal precision 10<sup>-4</sup>
  * 21 : real decimal precision 10<sup>-5</sup>
  * 22 : real decimal precision 10<sup>-6</sup>
  * 23 : real decimal precision 10<sup>-7</sup>
  * 24 : real decimal precision 10<sup>-8</sup>
  * 25 : real decimal precision 10<sup>-9</sup>
  * 256 : string SASS TSS\_STIME
  * 257 : string SASS TSS\_SDATE
  * 258 : string Marketfeed date
  * 259 : string Marketfeed time
  * 260 : string Marketfeed time seconds
  * 261 : string Marketfeed enumeration

This hinting feature allows TibMsg to represent SASS data without loss of
information.  Since TibrvMsg does not have hinting, it loses decimal precision
information when SASS is converted to it and it can't represent partial data
types.

## TibrvMsg message format

```console
                      NameSize        Type Size
|- MsgSize -|- Magic ---|  |- Name ----|  |  |- Data ----|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|00 00 00 13|99 55 ee aa|04|6e 61 6d 00|0c|05|12 34 56 78|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|---- Header : 8 bytes -|--- Field ----------------------|
```

* The first 4 bytes is the message size, which includes the
8 byte header of the message.

* The second 4 bytes is a magic number, it is always 0x9955eeaa.

* Field names have a maximum of 255 bytes, the null byte which terminates a C
  string is included in the size of the field name.  A zero size field name
  indicates the name is NULL.

* TibrvMsg field types:
  * BADDATA   = 0
  * RVMSG     = 1
  * SUBJECT   = 2
  * DATETIME  = 3
  * OPAQUE    = 7
  * STRING    = 8
  * BOOLEAN   = 9
  * IPDATA    = 10
  * INT       = 11
  * UINT      = 12
  * REAL      = 13
  * ENCRYPTED = 32
  * ARRAY\_I8  = 34
  * ARRAY\_U8  = 35
  * ARRAY\_I16 = 36
  * ARRAY\_U16 = 37
  * ARRAY\_I32 = 38
  * ARRAY\_U32 = 39
  * ARRAY\_I64 = 40
  * ARRAY\_U64 = 41
  * ARRAY\_F32 = 44
  * ARRAY\_F64 = 45

  The Tibco Tibrv API allows users to define their own field types and there
  may be other field types.  Since the size is always encoded, a field can be
  parsed without knowing what type of data it contains.  Unlike TibMsg, the
  array element type is encoded in the type of the field -- there is no hint
  field for partial and array types.

* The size includes the bytes encoding the size as well as the data, it is encoded as follows:

  * 1 byte  : 00 -> 0x79 (range 0 to 121) (example: 0x04)
  * 3 bytes : 0x79, 0 -> 0xffff (range 0 to 2<sup>16</sup>) (example: 0x79 0x12 0x34)
  * 5 bytes : 0x7a, 0 -> 0xffffffff (range 0 to 2<sup>32</sup>) (example: 0x7a 0x12 0x34 0x56 0x78)

This is an example of a Tibrv message within a Tibrv message:
  { "data" : { "field" : "value" } }.

```console
                      NameSize            Type Size
|- MsgSize -|- Magic ---|  |- Name -------|  |  0x7a = 4 byte size
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|00 00 00 1f|99 55 ee aa|05|64 61 74 61 00|01|7a 00 00 00 17|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|---- Header : 8b ------|    d  a  t  a \0|  |   MsgSize = 0x17 = 23b
                                                |---- Header

          NameSize              Type Size
|- Magic ---|  |- Name ----------|  |  |-- Data --------|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|99 55 ee aa|06|66 69 65 6c 64 00|08|07|76 61 6c 75 65 00|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
: 8b -------|    f  i  e  l  d \0|     | v  a  l  u  e \0|

```

The message size, 23 bytes is the length from the position after 0x7a.  A four
byte size is always used when the field is type MESSAGE so that it is
recursive:  a message always has a 4 byte size and a 4 byte magic, even when it
is a field value.

This is an example of a TibMsg within an opaque field "\_data\_" of a TibrvMsg,
which often occurs when a TibMsg is sent to a client from raicache over a
TRDP/RVD transport.

```console
                      NameSize                 Type Size
|- MsgSize -|- Magic ---|  |- Name -------------|  |  |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|00 00 00 2a|99 55 ee aa|07|5f 64 61 74 61 5f 00|07|20|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|---- Header : 8b ------|    _  d  a  t  a  _ \0| size = 0x20/32

                         NameSize              
|--- TibMsg |  | size = 23 |  |- Name -------------------|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|ce 13 aa 1f|01|00 00 00 17|09|54 52 44 50 52 43 5f 31 00|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|---- Header : 9b ---------|    T  R  D  P  R  C  _  1 \0

Type Size                    Hint Size Value
|  |  |---- Data -------------|  |  |  |
+--+--+--+--+--+--+--+--+--+--+--+--+--+
|47|08|3f f2 00 00 00 00 00 00|06|01|13|
+--+--+--+--+--+--+--+--+--+--+--+--+--+
  |  |        |
 real size 8, value = 1.125   hint = 0x13/19 (real decimal precision = 3)

TRDPRC_1       : REAL      8 : 1.125 <19>
```

## SASS QForm message format

```console
|- Magic ---|- MsgSize -| FID  Data | FID  Data | FID  Data | FID  Data |
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|11 11 11 12|00 00 00 10|cf a1|00 01|cf a2|8b 4f|cf a3|00 00|cf a5|00 00|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|---- Header : 8 bytes -|--- Fields ------------------------------------|
```

* The first 4 bytes is the magic number, always 0x11111112

* The second 4 bytes is the size of the data, it does not include the header
of the message

* The FID has two parts, the lower 14 bits is the dictionary fid, the upper 2
  bits are flags for FIXED (0x8000) and PRIMITIVE (0x4000).  The FIXED flags
  indicates it is a fixed size the PRIMITIVE flag indicates it is a field.
  These bits are always set on fields.  The above fids are 0xcfa1, 0xcfa2,
  0xcfa3, and 0xcfa5, which are usually defined in SYS\_tss\_fields.cf as fids
  4001 "MSG\_TYPE", 4002 "REC\_TYPE", 4003 "SEQ\_NO", and 4005 "REC\_STATUS".

* The data types defined are:
  * NODATA      =  0 (not used)
  * INTEGER     =  1
  * STRING      =  2
  * BOOLEAN     =  3 (not used)
  * DATE        =  4 (not used)
  * TIME        =  5 (not used)
  * PRICE       =  6 (not used)
  * BYTE        =  7 (not used)
  * FLOAT       =  8 (not used)
  * SHORT\_INT  =  9
  * DOUBLE      = 10 (not used)
  * OPAQUE      = 11 (not used)
  * NULL        = 12 (not used)
  * RESERVED    = 13 (not used)
  * DOUBLE\_INT = 14
  * GROCERY     = 15
  * SDATE       = 16
  * STIME       = 17
  * LONG        = 18 (not used)
  * U\_SHORT    = 19 (not used)
  * U\_INT      = 20 (not used)
  * U\_LONG     = 21 (not used)

When a FID is defined, it has one of the above types and a size.  Not all types
are supported and not all sizes of each type is supported (most will still work
even though they are not used).  These combinations are usually used when a
field is converted from RDMFieldDictionary:

 | RDM Field type + length | SASS Field type + length |
 | ----------------------- | ------------------------ |
 | ALPHANUMERIC + length   | STRING(2) + length |
 | TIME + 5                | STIME(17) + 6 |
 | DATE + 11               | SDATE(16) + 12 |
 | ENUMERATED + enum len   | STRING(2) + max( enum len, len ) + 1 |
 | INTEGER + len           | if ( len < 4 ) INTEGER(1) + 4 else DOUBLE\_INT(14) + 8 |
 | PRICE + len             | GROCERY(15) + 9 |
 | TIME\_SECONDS + len     | STIME(17) + 10 |
 | BINARY + len            | STRING(2) + len + 1 |

A field is aligned on even byte boundaries.  The GROCERY field which is always 
defined as a double and a precision hint takes up 9 bytes, but is aligned on
an even byte boundary, so it uses 10 bytes.

Here is a message with an example for each type of field.

```console
|- Magic ---|- MsgSize -| 
+--+--+--+--+--+--+--+--+
|11 11 11 12|00 00 00 58|
+--+--+--+--+--+--+--+--+

SYMBOL fid: 2705 size 20  (string 20)
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|ca 91|41 42 43 2e 4e 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        A  B  C  .  N

RDNDISPLAY fid: 10701 size: 4  (integer 4)
+--+--+--+--+--+--+
|e9 cd|00 00 00 40|
+--+--+--+--+--+--+

RDN_EXCHID fid: 10703 size 4  (enum -> string 4)
+--+--+--+--+--+--+
|e9 cf|4e 59 53 00|
+--+--+--+--+--+--+
        N  Y  S \0

TIMACT fid: 10704 size: 6  (time -> stime 6)
+--+--+--+--+--+--+--+--+
|e9 d0|31 39 3a 33 33 00|
+--+--+--+--+--+--+--+--+
        1  9  :  3  3 \0

ACTIV_DATE fid: 10716 size: 12  (date -> sdate 12)
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|e9 dc|31 38 20 4f 43 54 20 32 30 31 31 00|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        1  8     O  C  T     2  0  1  1 \0

ACVOL_1 fid: 10730 size: 8  (int -> double_int 8)
+--+--+--+--+--+--+--+--+--+--+
|e9 ea|40 8f 40 00 00 00 00 00|
+--+--+--+--+--+--+--+--+--+--+

BID fid: 10720, size: 9  (price -> grocery 9)
+--+--+--+--+--+--+--+--+--+--+--+--+
|e9 e0|3f f2 00 00 00 00 00 00|13 00| <- hint 0x13 = 19
+--+--+--+--+--+--+--+--+--+--+--+--+

BIDSIZE fid: 10728, size 8  (int -> double_int 8)
+--+--+--+--+--+--+--+--+--+--+
|e9 e8|40 24 00 00 00 00 00 00|
+--+--+--+--+--+--+--+--+--+--+

SYMBOL         : STRING   20 : "ABC.N"
RDNDISPLAY     : INT       4 : 64
RDN_EXCHID     : STRING    4 : "NYS"
TIMACT         : STRING    6 : "19:33" <257>
ACTIV_DATE     : STRING   12 : "18 OCT 2011" <256>
ACVOL_1        : REAL      8 : 1000.0 <0>
BID            : REAL      8 : 1.125 <19>
BIDSIZE        : REAL      8 : 10.0 <0>
```

