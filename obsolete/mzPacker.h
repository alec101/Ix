#pragma once



class mzPacker {
  void *_partDecomp, *_partComp;
  //bool _startDecomp;
  void *_file;
  size_t _nrBytes;
  uchar *_inBuffer, *_outBuffer;
  uchar *_pIn, *_pOut;
  bool _bMoreIn, _bMoreOut, _bAllIn;
  size_t _bytesProcessed;
  // uint _inLoc;
  size_t _inSize, _outSize;

public:

  char compressionLevel;
  void setCompressionLevel(char); /// 1- 10 ?
  void setDefaultCompressionLevel() { compressionLevel= 6; }

  // main compression / decompression functions
  
  bool compress(const void *src, uint64 srcLen, void *out, uint64 outLen);
  bool decompress(const void *src, uint64 srcLen, void *out, uint64 outLen);

  uint64 compressBound(uint64 srcSize);   /// max size of the compressed data
  uint64 decompressBound(uint64 srcSize); /// max size of the decompressed data

  // file comp / decomp

  void startFileDecomp(void *file, size_t nrBytes); /// preopen file, go to inside file position, specify how many bytes to decompile
  void *doFileDecomp(size_t *retSize= null);        /// returns pointer to decompr data; [retSize] will return nr bytes in returned pointer (this can be found in reports too)


  // compress / decompress RESULTS - check these vars after each compress / decompress operation

  struct _mzPackerResults {
    uint64 srcProcessed;      /// how many bytes were compressed/ decompressed in src buffer
    uint64 srcRemaining;      /// how many bytes remaining to compress / decompress in src buffer
    bool srcFullyProcessed;   /// src buffer FULLY processed (compress / decompress)

    uint64 outFilled;         /// how much of the out buffer was filled after a compress/decompress operation
    uint64 outRemaining;      /// how many bytes remaining in out buffer
    bool outIsFull;           /// out buffer is FULL

    _mzPackerResults();
    void delData();
  } results;

  // util funcs

  ulong crc32(ulong, const uint8 *, size_t);

  // error handling - if funcs do not return true, check err number / getErr in text

  uint err;
  cchar *getErr();

  // constructor / destructor

  mzPacker();
  ~mzPacker();
  void delData();





  // WIP vvv
  // part decompressing: call the ..start() func, then decompress same source, advancing the source pointer
  // THESE NEED FURTHER THINKING, the buffers must be power of 2
  void partDecompressStart();
  bool partDecompress(const void *src, uint64 srcLen, void *out, uint64 outLen, void *next);    /// fills out, with supplied src in partDecompressStart

  void partCompressStart();
  bool partCompress(const void *src, uint64 srcLen, void *out, uint64 outLen);      /// fills 

  

  void startComp(size_t nrBytes);
  void *doComp(void *src, size_t *retSize= null);   // ??
  // void *endComp(void *src, size_t *retSize= null);  // ??
  
  void startFileComp(void *file, size_t nrBytes);   /// preopen file (will write @ current file pos), specify how many bytes it will compress in total
  size_t doFileComp(void *buffer, size_t bufSize);  /// keep passing stuff to compress (in buffer) until specified size in startFileComp is reached; specify the buffer size too, in bufSize; returns nr bytes still required to compress
  // WIP ^^^



};




/* ERROR values ([err] var)

  multiple errors can be flagged in [err] << SUBJECT TO CHANGE, this might be an encuberance!!!!!!!!!!!!!!!!!!!

0000 = OK, no errors
0001 = failed (unknown reason)
0002 = source buffer error
0004 = output buffer error
0008 = func parameter error
000F = adler32 mismatch
0010 = more input needed
0020 = have more output data
0040 = 
0080 = 
00F0 = 
0100 = 
0200 = 
0400 = 
0800 = 
0F00 = 




*/





