#include <stdio.h>
#include <math.h>
#include <raimd/rwf_msg.h>
#include <raimd/md_dict.h>
#include <raimd/app_a.h>

using namespace rai;
using namespace md;

static const char summary_type[]       = "Type",
                  summary_dict_id[]    = "DictionaryId",
                  summary_version[]    = "Version",
                  summary_dt_version[] = "DT_Version",
                  summary_rt_version[] = "RT_Version";
/*
 * field dict summary:
 * series-summary     : message     44 : { 
 *     Type               : int          1 : 1
 *     DictionaryId       : int          1 : 1
 *     Version            : string       7 : "4.20.57"
 * }   
 * enum summary:
 * series-summary     : message     80 : {
 *     Type               : int          1 : 2
 *     DictionaryId       : int          1 : 1
 *     RT_Version         : string       7 : "4.20.57"
 *     DT_Version         : string       5 : "23.41"
 *     Version            : string       5 : "23.41"
 * }
 */

RwfSeriesWriter &
RwfSeriesWriter::encode_field_dictionary( MDFid start_fid,
                                          MDFid &end_fid,
                                          uint8_t verbosity,
                                          bool add_summary,
                                          size_t buflen ) noexcept
{
  end_fid = start_fid;
  if ( this->dict == NULL )
    return this->set_error( Err::NO_DICTIONARY );
  MDDict & d = *this->dict;
  RwfFieldDefnWriter & defn = this->add_field_defn();
  defn.add_defn( 1, DEFN_IS_ELEM_LIST )
      .append_defn  ( "NAME"      , RWF_ASCII_STRING )
      .append_defn  ( "FID"       , RWF_INT_2 )
      .append_defn  ( "RIPPLETO"  , RWF_INT_2 )
      .append_defn  ( "TYPE"      , RWF_INT_1 )
      .append_defn  ( "LENGTH"    , RWF_UINT_2 )
      .append_defn  ( "RWFTYPE"   , RWF_UINT_1 )
      .append_defn  ( "RWFLEN"    , RWF_UINT_2 );
  if ( verbosity >= DICT_VERB_NORMAL ) {
    defn.append_defn  ( "ENUMLENGTH", RWF_UINT_1 )
        .append_defn  ( "LONGNAME"  , RWF_ASCII_STRING );
  }
  defn.end_field_defn();
  if ( add_summary ) {
    const char * val;
    size_t       val_sz;
    uint32_t     id = 0;
    RwfElementListWriter &el = this->add_summary_element_list();

    el.append_int( summary_type, sizeof( summary_type ) - 1, 1 );
    if ( d.find_tag( summary_dict_id, val, val_sz ) ) {
      for ( size_t i = 0; i < val_sz; i++ ) {
        if ( val[ 0 ] < '0' || val[ 0 ] > '9' )
          break;
        id = ( id * 10 ) + ( val[ 0 ] - '0' );
      }
      if ( id != 0 )
        el.append_int( summary_dict_id, sizeof( summary_dict_id ) - 1, id );
    }
    if ( id == 0 )
      el.append_int( summary_dict_id, 1 );
    if ( d.find_tag( summary_version, val, val_sz ) )
      el.append_string( summary_version, sizeof( summary_version ) - 1,
                        val, val_sz );
    el.end_summary();
  }

  MDFid fid = start_fid;
  for ( ; fid <= d.max_fid; fid++ ) {
    MDLookup by( fid );
    const char * name, * ripple;
    uint8_t name_len, ripple_len, fname_len;
    uint8_t  rwf_type, mf_type;
    uint32_t mf_len, rwf_len, enum_len;
    MDFid    rip_fid = 0;

    if ( ! d.lookup( by ) )
      continue;

    by.mf_type( mf_type, mf_len, enum_len );
    by.rwf_type( rwf_type, rwf_len );
    by.get_name_ripple( name, name_len, ripple, ripple_len );
    if ( ripple != NULL ) {
      MDLookup rip( ripple, ripple_len );
      if ( d.get( rip ) )
        rip_fid = rip.fid;
    }
    fname_len = by.fname_len;
    if ( fname_len > 0 && by.fname[ fname_len - 1 ] == '\0' )
      fname_len--;
    if ( name_len > 0 && name[ name_len - 1 ] == '\0' )
      name_len--;

    RwfElementListWriter &el = this->add_element_list();
    el.use_field_set( 1 )
      .append_set_string( by.fname, fname_len )
      .append_set_int   ( (int16_t)  by.fid )
      .append_set_int   ( (int16_t)  rip_fid )
      .append_set_int   ( (int8_t)   mf_type )
      .append_set_uint  ( (uint16_t) mf_len )
      .append_set_uint  ( (uint8_t)  rwf_type )
      .append_set_uint  ( (uint16_t) rwf_len );
    if ( verbosity >= DICT_VERB_NORMAL ) {
      el.append_set_uint  ( (uint8_t) enum_len )
        .append_set_string( name, name_len );
    }
    el.end_entry();

    if ( this->off >= buflen )
      break;
  }
  end_fid = fid + 1;
  return *this;
}

RwfSeriesWriter &
RwfSeriesWriter::encode_enum_dictionary( uint32_t start_mapnum,
                                         uint32_t &end_mapnum,
                                         uint8_t /* verbosity */,
                                         bool add_summary,
                                         size_t buflen ) noexcept
{
  static const uint32_t MAX_FIDS   = 1024, /* max number of fids per enum */
                        MAX_VALUES = 4 * 1024; /* max num of values per enum */
  end_mapnum = start_mapnum;
  if ( this->dict == NULL )
    return this->set_error( Err::NO_DICTIONARY );
  MDDict & d = *this->dict;
  this->add_field_defn()
       .add_defn( 1, DEFN_IS_ELEM_LIST )
       .append_defn  ( "FIDS"      , RWF_ARRAY )
       .append_defn  ( "VALUE"     , RWF_ARRAY )
       .append_defn  ( "DISPLAY"   , RWF_ARRAY )
       .end_field_defn();
  if ( add_summary ) {
    const char * val;
    size_t       val_sz;
    uint32_t     id = 0;
    RwfElementListWriter &el = this->add_summary_element_list();

    el.append_int( summary_type, sizeof( summary_type ) - 1, 2 );
    if ( d.find_tag( summary_dict_id, val, val_sz ) ) {
      for ( size_t i = 0; i < val_sz; i++ ) {
        if ( val[ 0 ] < '0' || val[ 0 ] > '9' )
          break;
        id = ( id * 10 ) + ( val[ 0 ] - '0' );
      }
      if ( id != 0 )
        el.append_int( summary_dict_id, sizeof( summary_dict_id ) - 1, id );
    }
    if ( id == 0 )
      el.append_int( summary_dict_id, 1 );
    if ( d.find_tag( summary_rt_version, val, val_sz ) )
      el.append_string( summary_rt_version, sizeof( summary_rt_version ) - 1,
                        val, val_sz );
    if ( d.find_tag( summary_dt_version, val, val_sz ) ) {
      el.append_string( summary_dt_version, sizeof( summary_dt_version ) - 1,
                        val, val_sz );
      el.append_string( summary_version, sizeof( summary_version ) - 1,
                        val, val_sz ); /* vers == dt_ver for some reason */
    }
    else if ( d.find_tag( summary_version, val, val_sz ) ) {
      el.append_string( summary_version, sizeof( summary_version ) - 1,
                        val, val_sz );
    }
    el.end_summary();
  }
  uint32_t map_num = start_mapnum;
  for ( ; map_num < d.map_count; map_num++ ) {
    int16_t  fids[ MAX_FIDS ];
    uint32_t fid_cnt = 0;
    for ( MDFid fid = d.min_fid; fid <= d.max_fid; fid++ ) {
      MDLookup by( fid );

      if ( d.lookup( by ) && by.map_num == map_num && fid_cnt < MAX_FIDS )
        fids[ fid_cnt++ ] = fid;
    }
    if ( fid_cnt == 0 )
      continue;

    MDEnumMap * map = d.get_enum_map( map_num );
    uint16_t    values[ MAX_VALUES ];
    uint16_t  * val      = map->value();
    uint8_t   * disp     = map->map();
    uint32_t    val_cnt  = map->value_cnt,
                disp_len = map->max_len;

    if ( val == NULL ) {
      for ( uint32_t i = 0; i < val_cnt && i < MAX_VALUES; i++ )
        values[ i ] = i;
      val = values;
    }
    MDReference fids_m( fids, fid_cnt * sizeof( fids[ 0 ] ), MD_ARRAY,
                        MD_INT, sizeof( fids[ 0 ] ) ),
                values_m( val, val_cnt * sizeof( val[ 0 ] ), MD_ARRAY,
                          MD_ENUM, sizeof( val[ 0 ] ) ),
                display_m( disp, val_cnt * disp_len, MD_ARRAY, MD_STRING,
                           disp_len );
    this->add_element_list()
         .use_field_set( 1 )
         .append_set_ref( fids_m )
         .append_set_ref( values_m )
         .append_set_ref( display_m )
         .end_entry();

    if ( this->off >= buflen )
      break;
  }
  end_mapnum = map_num + 1;
  return *this;
}

namespace {
static const uint32_t FIELD_DICT_COUNT = 9;
struct FieldDictionaryEntry {
  char     name[ 256 ];
  int16_t  fid,
           rippleto;
  uint8_t  type,
           rwftype;
  uint16_t length,
           rwflen,
           enumlength;
  char     longname[ 256 ];

  void zero( void ) {
    this->name[ 0 ] = '\0';
    this->fid = this->rippleto = 0;
    this->type = this->rwftype = 0;
    this->length = this->rwflen = this->enumlength = 0;
    this->longname[ 0 ] = '\0';
  }
  void iter_map( MDIterMap *mp ) {
    mp[ 0 ].string( "NAME"      , name       , sizeof( name ) );
    mp[ 1 ].sint  ( "FID"       , &fid       , sizeof( fid ) );
    mp[ 2 ].sint  ( "RIPPLETO"  , &rippleto  , sizeof( rippleto ) );
    mp[ 3 ].uint  ( "TYPE"      , &type      , sizeof( type ) );
    mp[ 4 ].uint  ( "LENGTH"    , &length    , sizeof( length ) );
    mp[ 5 ].uint  ( "RWFTYPE"   , &rwftype   , sizeof( rwftype ) );
    mp[ 6 ].uint  ( "RWFLEN"    , &rwflen    , sizeof( rwflen ) );
    mp[ 7 ].uint  ( "ENUMLENGTH", &enumlength, sizeof( enumlength ) );
    mp[ 8 ].string( "LONGNAME"  , longname   , sizeof( longname ) );
  }
};
}

int
RwfMsg::decode_field_dictionary( MDDictBuild &dict_build,
                                 RwfMsg &series ) noexcept
{
  MDMsg       * elem_list;
  MDFieldIter * entries,
              * sumit;
  MDReference   mref;
  MDIterMap     map[ FIELD_DICT_COUNT ];
  size_t        field_cnt = 0;

  FieldDictionaryEntry fields;
  int status;

  fields.zero();
  fields.iter_map( map );
  if ( (status = series.get_field_iter( entries )) != 0 ||
       (status = entries->first()) != 0 )
    return status;

  if ( ((RwfFieldIter *) entries)->u.series.is_summary ) {
    if ( entries->get_reference( mref ) == 0 &&
         series.get_sub_msg( mref, elem_list, entries ) == 0 &&
         elem_list->get_field_iter( sumit ) == 0 ) {
      char idbuf[ 8 ];
      size_t idlen = sizeof( idbuf );
      if ( sumit->find( summary_dict_id, mref ) == 0 &&
           to_string( mref, idbuf, idlen ) == 0 ) {
        dict_build.add_tag( summary_dict_id, sizeof( summary_dict_id ) - 1,
                            idbuf, idlen );
      }
      if ( sumit->find( summary_version, mref ) == 0 &&
           mref.ftype == MD_STRING ) {
        dict_build.add_tag( summary_version, sizeof( summary_version ) - 1,
                            (char *) mref.fptr, mref.fsize );
      }
    }
    if ( (status = entries->next()) != 0 )
      return status;
  }
  do {
    if ( (status = entries->get_reference( mref )) != 0 ||
         (status = series.get_sub_msg( mref, elem_list, entries )) != 0 )
      return status;
    MDIterMap::get_map( *elem_list, map, FIELD_DICT_COUNT );
    dict_build.add_rwf_entry( fields.name, fields.longname,
                              fields.fid, fields.rippleto,
                              fields.type, fields.length,
                              fields.enumlength, fields.rwftype,
                              fields.rwflen, NULL, field_cnt++ );
    fields.zero();
  } while ( (status = entries->next()) == 0 );
  if ( status == Err::NOT_FOUND )
    return 0;
  return status;
}

namespace {
static const uint32_t FIELD_ENUM_COUNT = 3;
struct EnumDictionaryEntry {
  int16_t  fids[ 1024 ];              /* max num fids 1024 */
  uint16_t values[ 4 * 1024 ];        /* max num values 4096 */
  char     display[ 4 * 1024 ][ 64 ]; /* max value len 64 */
  uint32_t num_fids,
           num_values,
           num_display;

  void zero( void ) {
    this->num_fids = this->num_values = this->num_display = 0;
  }
  void iter_map( MDIterMap *mp ) {
    mp[ 0 ].sint_array  ( "FIDS"   , fids   , &num_fids   , sizeof( fids )   , sizeof( fids[ 0 ] ) );
    mp[ 1 ].uint_array  ( "VALUE"  , values , &num_values , sizeof( values ) , sizeof( values[ 0 ] ) );
    mp[ 2 ].string_array( "DISPLAY", display, &num_display, sizeof( display ), sizeof( display[ 0 ] ) );
  }
};
}

int
RwfMsg::decode_enum_dictionary( MDDictBuild &dict_build,
                                RwfMsg &series ) noexcept
{
  MDMsg       * elem_list;
  MDFieldIter * entries,
              * sumit;
  MDReference   mref;
  MDIterMap     map[ FIELD_ENUM_COUNT ];
  size_t        enum_cnt = 0;

  EnumDictionaryEntry  enums;
  int status;

  enums.zero();
  enums.iter_map( map );
  if ( (status = series.get_field_iter( entries )) != 0 ||
       (status = entries->first()) != 0 )
    return status;

  if ( ((RwfFieldIter *) entries)->u.series.is_summary ) {
    if ( entries->get_reference( mref ) == 0 &&
         series.get_sub_msg( mref, elem_list, entries ) == 0 &&
         elem_list->get_field_iter( sumit ) == 0 ) {
      if ( sumit->find( summary_rt_version, mref ) == 0 &&
           mref.ftype == MD_STRING ) {
        dict_build.add_tag( summary_rt_version, sizeof( summary_rt_version ) -1,
                            (char *) mref.fptr, mref.fsize );
      }
      if ( sumit->find( summary_dt_version, mref ) == 0 &&
           mref.ftype == MD_STRING ) {
        dict_build.add_tag( summary_dt_version, sizeof( summary_dt_version ) -1,
                            (char *) mref.fptr, mref.fsize );
      }
    }
    if ( (status = entries->next()) != 0 )
      return status;
  }
  do {
    if ( (status = entries->get_reference( mref )) != 0 ||
         (status = series.get_sub_msg( mref, elem_list, entries )) != 0 )
      return status;
    MDIterMap::get_map( *elem_list, map, FIELD_ENUM_COUNT );
    dict_build.add_rwf_enum_map( enums.fids, enums.num_fids,
                                 enums.values, enums.num_values,
                                 (char *) enums.display,
                                 sizeof( enums.display[ 0 ] ),
                                 enum_cnt++ );

    enums.zero();
  } while ( (status = entries->next()) == 0 );
  if ( status == Err::NOT_FOUND )
    return 0;
  return status;
}

