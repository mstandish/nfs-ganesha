/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright CEA/DAM/DIF  (2011)
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * ---------------------------------------
 */

/**
 * \file    9p_lopen.c
 * \brief   9P version
 *
 * 9p_lopen.c : _9P_interpretor, request LOPEN
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include "nfs_core.h"
#include "log.h"
#include "cache_inode.h"
#include "fsal.h"
#include "9p.h"


int _9p_lopen( _9p_request_data_t * preq9p, 
               void  * pworker_data,
               u32 * plenout, 
               char * preply)
{
  char * cursor = preq9p->_9pmsg + _9P_HDR_SIZE + _9P_TYPE_SIZE ;

  u16 * msgtag = NULL ;
  u32 * fid    = NULL ;
  u32 * flags  = NULL ;

  cache_inode_status_t cache_status ;
  fsal_openflags_t openflags = 0 ;

  if( !preq9p || !pworker_data || !plenout || !preply )
   return -1 ;

  _9p_fid_t * pfid = NULL ;

  /* Get data */
  _9p_getptr( cursor, msgtag, u16 ) ; 
  _9p_getptr( cursor, fid,    u32 ) ; 
  _9p_getptr( cursor, flags,  u32 ) ; 
  
  LogDebug( COMPONENT_9P, "TLOPEN: tag=%u fid=%u flags=0x%x",
            (u32)*msgtag, *fid, *flags  ) ;

   if( *fid >= _9P_FID_PER_CONN )
     return _9p_rerror( preq9p, msgtag, ERANGE, plenout, preply ) ;
 
   pfid =  &preq9p->pconn->fids[*fid] ;

  _9p_openflags2FSAL( flags, &openflags ) ; 

  if( pfid->pentry->type == REGULAR_FILE ) /** @todo: Maybe other types (FIFO, sOCKET,...) may require to be opened too */
   {
      if(cache_inode_open( pfid->pentry, 
                           openflags, 
                           &pfid->fsal_op_context,
                           0, 
                           &cache_status) != CACHE_INODE_SUCCESS) 
         return _9p_rerror( preq9p, msgtag, _9p_tools_errno( cache_status ), plenout, preply ) ;
   }

   /* iounit = 0 by default */
   pfid->specdata.iounit = 0 ;

   /* Build the reply */
  _9p_setinitptr( cursor, preply, _9P_RLOPEN ) ;
  _9p_setptr( cursor, msgtag, u16 ) ;

  _9p_setqid( cursor, pfid->qid ) ;
  _9p_setptr( cursor, &pfid->specdata.iounit, u32 ) ; 
   
  _9p_setendptr( cursor, preply ) ;
  _9p_checkbound( cursor, preply, plenout ) ;

  LogDebug( COMPONENT_9P, "RLOPEN: tag=%u fid=%u qid=(type=%u,version=%u,path=%llu) iounit=%u", 
            *msgtag, *fid, (u32)pfid->qid.type, pfid->qid.version, (unsigned long long)pfid->qid.path, pfid->specdata.iounit ) ;

  return 1 ;
}

