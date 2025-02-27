#ifndef SQV_ERR_HEADER_
#define SQV_ERR_HEADER_

typedef enum {
  SQV__UNKNOWN = -1,

  SQV_SUCCESS,

  QK_MDL_FILE_OPEN,
  QK_MDL_MEM_ALLOC,
  QK_MDL_READ_SIZE,
  QK_MDL_INVALID,

  SQV__MAX = 256
} sqv_err;

#endif  // SQV_ERR_HEADER_
