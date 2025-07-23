package main

/*
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <raimd/md_replay.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/sass.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>
#include <raimd/rwf_writer.h>
#include <raimd/md_hash_tab.h>
#include <raimd/dict_load.h>
#include <raimd/md_msg.h>
#include <raimd/md_types.h>
#include <raimd/md_field_iter.h>

// Helper functions to access the const char arrays
static inline const char* get_sass_msg_type() { return MD_SASS_MSG_TYPE; }
static inline const char* get_sass_rec_type() { return MD_SASS_REC_TYPE; }
static inline const char* get_sass_seq_no() { return MD_SASS_SEQ_NO; }
static inline size_t get_sass_msg_type_len() { return MD_SASS_MSG_TYPE_LEN; }
static inline size_t get_sass_rec_type_len() { return MD_SASS_REC_TYPE_LEN; }
static inline size_t get_sass_seq_no_len() { return MD_SASS_SEQ_NO_LEN; }
*/
import "C"

import (
	"fmt"
	"os"
	"strings"
	"unsafe"
)

// MetaData represents the metadata structure from C
type MetaData struct {
	Subject  string
	RecType  uint16
	FList    uint16
	SeqNo    uint32
	FormPtr  unsafe.Pointer // MDFormClass_t*
}

// SubjectHashMap is a Go map equivalent to the C hash table
type SubjectHashMap map[string]*MetaData

func getArg(args []string, offset int, flag string, defaultVal string) string {
	for i := 1; i < len(args)-offset; i++ {
		if args[i] == flag && i+offset < len(args) {
			return args[i+offset]
		}
	}
	return defaultVal
}

func hasFlag(args []string, flag string) bool {
	for i := 1; i < len(args); i++ {
		if args[i] == flag {
			return true
		}
	}
	return false
}

func main() {
	var (
		mout        *C.MDOutput_t
		bout        *C.MDOutput_t
		mem         *C.MDMsgMem_t
		dict        *C.MDDict_t
		cfileDict   *C.MDDict_t
		rdmDict     *C.MDDict_t
		flistDict   *C.MDDict_t
		replay      *C.MDReplay_t
		msgCount    uint64
		errCnt      uint64
		discardCnt  uint64
		cvtTypeID   uint32
	)

	args := os.Args
	fn := getArg(args, 1, "-f", "")
	if fn == "" {
		fn = "-" // stdin
	}
	out := getArg(args, 1, "-o", "")
	path := getArg(args, 1, "-p", os.Getenv("cfile_path"))
	format := getArg(args, 1, "-t", "")
	quiet := hasFlag(args, "-q")
	binout := false

	// Initialize C structures
	C.md_output_init(&mout)
	C.md_output_init(&bout)
	if !C.md_msg_mem_create(&mem) {
		fmt.Fprintf(os.Stderr, "Failed to create memory\n")
		os.Exit(1)
	}

	if hasFlag(args, "-h") {
		fmt.Fprintf(os.Stderr, `Usage: %s [-f file] [-o out] [-p cfile_path] [-r flds] [-k flds] [-q]
Test reading messages from files on command line
  -f file           = replay file to read
  -o file           = output file to write
  -p cfile_path     = load dictionary files from path
  -t format         = convert message to (eg TIBMSG,RVMSG,RWFMSG)
  -q                = quiet, no printing of messages
A file is a binary dump of messages, each with these lines:
<subject>
<num-bytes>
<blob-of-data

Example:
test.subject
15
{ "ten" : 10 }
`, args[0])
		os.Exit(1)
	}

	// Load dictionaries
	if path != "" {
		cPath := C.CString(path)
		defer C.free(unsafe.Pointer(cPath))
		dict = C.md_load_dict_files(cPath, C.bool(true))
	}

	if dict != nil {
		for d := dict; d != nil; d = d.next {
			dictType := C.GoString(&d.dict_type[0])
			if len(dictType) > 0 {
				switch dictType[0] {
				case 'c':
					cfileDict = d
				case 'a':
					rdmDict = d
				case 'f':
					flistDict = d
				}
			}
		}
	} else {
		fmt.Fprintf(os.Stderr, "no dictionary found (%s)\n", path)
	}

	// Handle output file
	if out != "" {
		if out == "-" {
			quiet = true
		} else {
			cOut := C.CString(out)
			cMode := C.CString("wb")
			defer C.free(unsafe.Pointer(cOut))
			defer C.free(unsafe.Pointer(cMode))
			if C.md_output_open(bout, cOut, cMode) != 0 {
				fmt.Fprintf(os.Stderr, "open error %s\n", out)
				os.Exit(1)
			}
		}
		binout = true
	}

	C.md_init_auto_unpack()

	// Handle format conversion
	if format != "" {
		var i C.uint32_t
		for m := C.md_msg_first_match(&i); m != nil; m = C.md_msg_next_match(&i) {
			name := C.GoString(m.name)
			if strings.EqualFold(name, format) {
				cvtTypeID = uint32(m.hint[0])
				break
			}
		}

		switch cvtTypeID {
		case C.JSON_TYPE_ID, C.MARKETFEED_TYPE_ID, C.RVMSG_TYPE_ID,
			 C.RWF_FIELD_LIST_TYPE_ID, C.RWF_MSG_TYPE_ID, C.TIBMSG_TYPE_ID, C.TIB_SASS_TYPE_ID:
			// Valid types
		default:
			status := "not found"
			if cvtTypeID != 0 {
				status = "converter not implemented"
			}
			fmt.Fprintf(os.Stderr, "format \"%s\" %s, types:\n", format, status)
			var i C.uint32_t
			for m := C.md_msg_first_match(&i); m != nil; m = C.md_msg_next_match(&i) {
				if m.hint[0] != 0 {
					name := C.GoString(m.name)
					fmt.Fprintf(os.Stderr, "  %s : %x\n", name, m.hint[0])
				}
			}
			os.Exit(1)
		}
	}

	// Create replay
	if !C.md_replay_create(&replay, nil) {
		fmt.Fprintf(os.Stderr, "Failed to create replay\n")
		os.Exit(1)
	}

	// Open replay file
	cFn := C.CString(fn)
	if !C.md_replay_open(replay, cFn) {
		fmt.Fprintf(os.Stderr, "open %s failed\n", fn)
		C.free(unsafe.Pointer(cFn))
		C.md_replay_destroy(replay)
		os.Exit(1)
	}
	C.free(unsafe.Pointer(cFn))

	if !quiet {
		filename := fn
		if fn == "-" {
			filename = "stdin"
		}
		fmt.Printf("reading from %s\n", filename)
	}

	// Create Go map instead of C hash table
	subMap := make(SubjectHashMap)

	// Main message processing loop
	for b := C.md_replay_first(replay); b; b = C.md_replay_next(replay) {
		// Get subject string
		subjLen := int(replay.subjlen)
		subj := strings.TrimSpace(C.GoStringN(replay.subj, C.int(subjLen)))

		// Reset memory for next message
		C.md_msg_mem_reuse(mem)

		// Unpack message
		m := C.md_msg_unpack(unsafe.Pointer(replay.msgbuf), 0, replay.msglen, 0, dict, mem)
		msg := unsafe.Pointer(replay.msgbuf)
		msgSz := replay.msglen
		status := -1

		if m == nil {
			errCnt++
			continue
		}

		// Handle format conversion
		if cvtTypeID != 0 && m != nil {
			var form *C.MDFormClass_t
			bufSz := msgSz
			if ( bufSz < 1024 ) {
				bufSz = 1024 
			}
			bufPtr := C.md_msg_mem_make(mem, C.size_t(bufSz))
			var flist uint16
			msgType := uint16(C.MD_UPDATE_TYPE)
			var recType uint16
			var seqno uint32

			// Get message type specific data
			switch C.md_msg_get_type_id(m) {
			case C.MARKETFEED_TYPE_ID:
				C.md_msg_get_sass_msg_type(m, (*C.uint16_t)(unsafe.Pointer(&msgType)))
				C.md_msg_mf_get_flist(m, (*C.uint16_t)(unsafe.Pointer(&flist)))
				C.md_msg_mf_get_rtl(m, (*C.uint32_t)(unsafe.Pointer(&seqno)))

			case C.RWF_MSG_TYPE_ID:
				C.md_msg_get_sass_msg_type(m, (*C.uint16_t)(unsafe.Pointer(&msgType)))
				fl := C.md_msg_rwf_get_container_msg(m)
				C.md_msg_rwf_get_flist(fl, (*C.uint16_t)(unsafe.Pointer(&flist)))
				C.md_msg_rwf_get_msg_seqnum(m, (*C.uint32_t)(unsafe.Pointer(&seqno)))

			case C.RWF_FIELD_LIST_TYPE_ID:
				C.md_msg_rwf_get_flist(m, (*C.uint16_t)(unsafe.Pointer(&flist)))

			default:
				// Handle other message types with field reader
				var rd *C.MDFieldReader_t
				rd = C.md_msg_get_field_reader(m)
				if C.md_field_reader_find(rd, C.get_sass_msg_type(), C.get_sass_msg_type_len()) {
					C.md_field_reader_get_uint(rd, unsafe.Pointer(&msgType), C.size_t(2))
				}
				if C.md_field_reader_find(rd, C.get_sass_rec_type(), C.get_sass_rec_type_len()) {
					C.md_field_reader_get_uint(rd, unsafe.Pointer(&recType), C.size_t(2))
				}
				if C.md_field_reader_find(rd, C.get_sass_seq_no(), C.get_sass_seq_no_len()) {
					C.md_field_reader_get_uint(rd, unsafe.Pointer(&seqno), C.size_t(4))
				}
			}

			status = -1

			// Handle TIBMSG and TIB_SASS conversion
			if cvtTypeID == C.TIBMSG_TYPE_ID || cvtTypeID == C.TIB_SASS_TYPE_ID {
				// Simplified version without full dictionary lookup
				if flist != 0 && flistDict != nil && cfileDict != nil {
					var by *C.MDLookup_t
					C.md_lookup_create_by_fid((**C.MDLookup_t)(unsafe.Pointer(&by)), C.MDFid(flist))
					defer C.md_lookup_destroy(by)
					if C.md_dict_lookup(flistDict, by) {
						var fc *C.MDLookup_t
						C.md_lookup_create_by_name((**C.MDLookup_t)(unsafe.Pointer(&fc)), by.fname, C.size_t(by.fname_len))
						defer C.md_lookup_destroy(fc)
						if C.md_dict_get(cfileDict, fc) {
							if fc.ftype == C.MD_MESSAGE {
								recType = uint16(fc.fid)
								if fc.map_num != 0 {
									form = C.md_dict_get_form_class(cfileDict, fc)
								} else {
									form = nil
								}
							}
						}

						data, exists := subMap[subj]
						if !exists {
							seqno = 1
							subMap[subj] = &MetaData{
								Subject: subj,
								RecType: recType,
								FList:   flist,
								SeqNo:   seqno,
								FormPtr: unsafe.Pointer(form),
							}
						} else {
							data.RecType = recType
							data.FList = flist
							data.FormPtr = unsafe.Pointer(form)
							data.SeqNo++
							seqno = data.SeqNo
						}
					}
				} else {
					// Use existing data from Go map
					if data, exists := subMap[subj]; exists {
						recType = data.RecType
						flist = data.FList
						form = (*C.MDFormClass_t)(data.FormPtr)
						data.SeqNo++
						seqno = data.SeqNo
					}
				}
			} else if cvtTypeID == C.RWF_MSG_TYPE_ID || cvtTypeID == C.RWF_FIELD_LIST_TYPE_ID {
				if recType != 0 && flistDict != nil && cfileDict != nil {
					var by *C.MDLookup_t
					C.md_lookup_create_by_fid((**C.MDLookup_t)(unsafe.Pointer(&by)), C.MDFid(recType))
					defer C.md_lookup_destroy(by)
					
					if C.md_dict_lookup(cfileDict, by) && by.ftype == C.MD_MESSAGE {
						var fl *C.MDLookup_t
						C.md_lookup_create_by_name((**C.MDLookup_t)(unsafe.Pointer(&fl)), by.fname, C.size_t(by.fname_len))
						defer C.md_lookup_destroy(fl)
						if C.md_dict_get(flistDict, fl) {
							flist = uint16(fl.fid)
						}
					}
				}
			}

			// Initialize sequence number if needed (matches C logic for seqno == 0 || cvt_type_id != 0)
			if seqno == 0 || cvtTypeID != 0 {
				data, exists := subMap[subj]
				if !exists {
					seqno++
					subMap[subj] = &MetaData{
						Subject: subj,
						RecType: recType,
						FList:   flist,
						SeqNo:   seqno,
						FormPtr: unsafe.Pointer(form),
					}
				} else {
					if recType == 0 {
						recType = data.RecType
					}
					if flist == 0 {
						flist = data.FList
					}
					if form == nil {
						form = (*C.MDFormClass_t)(data.FormPtr)
					}
					if seqno == 0 {
						data.SeqNo++
						seqno = data.SeqNo
					}
				}
			}

			// Message conversion based on type
			switch cvtTypeID {
			case C.JSON_TYPE_ID:
				w := C.json_msg_writer_create(mem, nil, bufPtr, bufSz)
				if C.md_msg_writer_convert_msg(w, m, C.bool(false)) == 0 {
					msg = unsafe.Pointer(w.buf)
					msgSz = C.size_t(C.md_msg_writer_update_hdr(w))
					m = C.json_msg_unpack(msg, 0, msgSz, 0, dict, mem)
				}
				status = 0

			case C.MARKETFEED_TYPE_ID:
				status = 0

			case C.RVMSG_TYPE_ID:
				w := C.rv_msg_writer_create(mem, nil, bufPtr, bufSz)
				cSubj := C.CString(subj)
				defer C.free(unsafe.Pointer(cSubj))
				C.md_msg_writer_append_sass_hdr(w, form, C.uint16_t(msgType), C.uint16_t(recType), C.uint16_t(seqno), 0, cSubj, C.size_t(len(subj)))
				if C.md_msg_writer_convert_msg(w, m, C.bool(false)) == 0 {
					msg = unsafe.Pointer(w.buf)
					msgSz = C.size_t(C.md_msg_writer_update_hdr(w))
					m = C.rv_msg_unpack(msg, 0, msgSz, 0, dict, mem)
				}
				status = 0

			case C.RWF_MSG_TYPE_ID:
				var msgClass C.RwfMsgClass
				if msgType == C.MD_INITIAL_TYPE {
					msgClass = C.REFRESH_MSG_CLASS
				} else {
					msgClass = C.UPDATE_MSG_CLASS
				}
				cSubjForHash := C.CString(subj)
				streamID := C.md_dict_hash(cSubjForHash, C.size_t(len(subj)))
				C.free(unsafe.Pointer(cSubjForHash))
				w := C.rwf_msg_writer_create(mem, rdmDict, bufPtr, bufSz, msgClass, C.MARKET_PRICE_DOMAIN, streamID)

				if msgClass == C.REFRESH_MSG_CLASS {
					C.md_msg_writer_rwf_set(w, C.X_CLEAR_CACHE)
					C.md_msg_writer_rwf_set(w, C.X_REFRESH_COMPLETE)
				}
				C.md_msg_writer_rwf_add_seq_num(w, C.uint32_t(seqno))
				cSubj := C.CString(subj)
				defer C.free(unsafe.Pointer(cSubj))
				C.md_msg_writer_rwf_add_msg_key(w, cSubj, C.size_t(len(subj)))

				fl := C.md_msg_writer_rwf_add_field_list(w)
				if flist != 0 {
					C.md_msg_writer_rwf_add_flist(fl, C.uint16_t(flist))
				}

				status = int(w.err)
				if status == 0 {
					status = int(C.md_msg_writer_convert_msg(fl, m, C.bool(true)))
				}
				if status == 0 {
					C.md_msg_writer_rwf_end_msg(w)
				}
				if status = int(w.err); status == 0 {
					msg = unsafe.Pointer(w.buf)
					msgSz = C.size_t(w.off)
					m = C.rwf_msg_unpack(msg, 0, msgSz, 0, rdmDict, mem)
				}

			case C.RWF_FIELD_LIST_TYPE_ID:
				w := C.rwf_msg_writer_field_list_create(mem, rdmDict, bufPtr, bufSz)
				if flist != 0 {
					C.md_msg_writer_rwf_add_flist(w, C.uint16_t(flist))
				}
				if C.md_msg_writer_convert_msg(w, m, C.bool(false)) == 0 {
					msg = unsafe.Pointer(w.buf)
					msgSz = C.size_t(C.md_msg_writer_update_hdr(w))
					m = C.rwf_msg_unpack_field_list(msg, 0, msgSz, 0, rdmDict, mem)
				}
				status = 0

			case C.TIBMSG_TYPE_ID:
				w := C.tib_msg_writer_create(mem, nil, bufPtr, bufSz)
				cSubj := C.CString(subj)
				defer C.free(unsafe.Pointer(cSubj))
				C.md_msg_writer_append_sass_hdr(w, form, C.uint16_t(msgType), C.uint16_t(recType), C.uint16_t(seqno), 0, cSubj, C.size_t(len(subj)))
				if C.md_msg_writer_convert_msg(w, m, C.bool(true)) == 0 {
					msg = unsafe.Pointer(w.buf)
					msgSz = C.size_t(C.md_msg_writer_update_hdr(w))
					m = C.tib_msg_unpack(msg, 0, msgSz, 0, cfileDict, mem)
				}
				status = 0

			case C.TIB_SASS_TYPE_ID:
				if form != nil {
					w := C.tib_sass_msg_writer_create_with_form(mem, form, bufPtr, bufSz)
					cSubj := C.CString(subj)
					defer C.free(unsafe.Pointer(cSubj))
					C.md_msg_writer_append_sass_hdr(w, form, C.uint16_t(msgType), C.uint16_t(recType), C.uint16_t(seqno), 0, cSubj, C.size_t(len(subj)))
					if msgType == C.MD_INITIAL_TYPE {
						C.md_msg_writer_append_form_record(w)
					}
					if C.md_msg_writer_convert_msg(w, m, C.bool(true)) == 0 {
						msg = unsafe.Pointer(w.buf)
						msgSz = C.size_t(C.md_msg_writer_update_hdr(w))
						m = C.tib_sass_msg_unpack(msg, 0, msgSz, 0, cfileDict, mem)
					}
				} else {
					w := C.tib_sass_msg_writer_create(mem, cfileDict, bufPtr, bufSz)
					cSubj := C.CString(subj)
					defer C.free(unsafe.Pointer(cSubj))
					C.md_msg_writer_append_sass_hdr(w, form, C.uint16_t(msgType), C.uint16_t(recType), C.uint16_t(seqno), 0, cSubj, C.size_t(len(subj)))
					if C.md_msg_writer_convert_msg(w, m, C.bool(true)) == 0 {
						msg = unsafe.Pointer(w.buf)
						msgSz = C.size_t(C.md_msg_writer_update_hdr(w))
						m = C.tib_sass_msg_unpack(msg, 0, msgSz, 0, cfileDict, mem)
					}
				}
				status = 0
			}

			if status != 0 || m == nil {
				if m != nil {
					fmt.Printf("%s: seqno %d error %d\n", subj, seqno, status)
					C.md_msg_print(m, mout)
					C.md_output_flush(mout)
				}
				errCnt++
				continue
			}
		}

		// Print message if not quiet
		if !quiet && m != nil {
			protoStr := C.GoString(C.md_msg_get_proto_string(m))
			fmt.Printf("\n## %s (fmt %s)\n", subj, protoStr)
			C.md_msg_print(m, mout)
			C.md_output_flush(mout)
		}

		// Write binary output
		if binout {
			if m != nil {
				t := C.md_msg_get_type_id(m)
				if (t == C.RWF_FIELD_LIST_TYPE_ID || t == C.RWF_MSG_TYPE_ID) &&
					C.get_u32_md_big(msg) != C.RWF_FIELD_LIST_TYPE_ID {
					// Create 8-byte header buffer
					buf := make([]byte, 8)
					C.set_u32_md_big(unsafe.Pointer(&buf[0]), C.RWF_FIELD_LIST_TYPE_ID)
					C.set_u32_md_big(unsafe.Pointer(&buf[4]), C.md_msg_rwf_get_base_type_id(m))
					subjHeader := fmt.Sprintf("%s\n%d\n", subj, msgSz + 8)
					cSubjHeader := C.CString(subjHeader)
					C.md_output_write(bout, unsafe.Pointer(cSubjHeader), C.size_t(len(subjHeader)))
					C.free(unsafe.Pointer(cSubjHeader))
					C.md_output_write(bout, unsafe.Pointer(&buf[0]), 8)
				} else {
					subjHeader := fmt.Sprintf("%s\n%d\n", subj, msgSz)
					cSubjHeader := C.CString(subjHeader)
					C.md_output_write(bout, unsafe.Pointer(cSubjHeader), C.size_t(len(subjHeader)))
					C.free(unsafe.Pointer(cSubjHeader))
				}
				if C.md_output_write(bout, msg, msgSz) != msgSz {
					errCnt++
				}
			} else {
				discardCnt++
			}
		}
		msgCount++
	}

	fmt.Fprintf(os.Stderr, "found %d messages, %d errors\n", msgCount, errCnt)
	if discardCnt > 0 {
		fmt.Fprintf(os.Stderr, "discarded %d\n", discardCnt)
	}

	if C.md_output_close(mout) != 0 {
		errCnt++
	}
	if C.md_output_close(bout) != 0 {
		errCnt++
	}

	C.md_replay_close(replay)
	C.md_replay_destroy(replay)
	C.md_msg_mem_destroy(mem)

	if errCnt == 0 {
		os.Exit(0)
	} else {
		os.Exit(2)
	}
}
