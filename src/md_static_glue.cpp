#if ! defined( MDMSG_DLL_EXP ) && defined( RAI_DLL )
#define MDMSG_DLL_EXP __declspec(dllexport) 
#endif

#include "mdmsg/md_msg.h"

extern "C" void RV_MSG_MDInitialize( void );
extern "C" void TIB_MSG_MDInitialize( void );
extern "C" void TIB_SASS_MSG_MDInitialize( void );
extern "C" void MF_MSG_MDInitialize( void );
#ifdef HAVE_UPA
extern "C" void RWF_MSG_MDInitialize( void );
#endif
extern "C" void OPEN_MSG_MDInitialize( void );
extern "C" void SOL_MSG_MDInitialize( void );
extern "C" void JSON_MSG_MDInitialize( void );

extern "C" RAI_DLL_EXPORT void
MD_MSG_MDInitialize( void )
{
  TIB_MSG_MDInitialize();
  TIB_SASS_MSG_MDInitialize();
  RV_MSG_MDInitialize();
  MF_MSG_MDInitialize();
#ifdef HAVE_UPA
  RWF_MSG_MDInitialize();
#endif
  OPEN_MSG_MDInitialize();
  SOL_MSG_MDInitialize();
  JSON_MSG_MDInitialize();
}

