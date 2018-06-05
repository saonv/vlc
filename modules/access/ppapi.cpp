/**
 * @file ppapi-access.cpp
 * @brief An access module for remote resources (ie playing a video/music from
 * an URL).
 */
/*****************************************************************************
 * Copyright © 2015 Cadonix, Richard Diamond
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

// XXX: Handle remote file updates.

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#include <queue>
#include <map>
#include <string>
#include <algorithm>
#include <unordered_map>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_ppapi.h>
#include <vlc_access.h>
#include <vlc_atomic.h>
#include <vlc_md5.h>
#include <vlc_input.h>
#include <vlc_messages.h>

#include <ppapi/c/pp_instance.h>
#include <ppapi/c/pp_completion_callback.h>
#include <ppapi/c/pp_directory_entry.h>
  


struct access_sys_t;
class LoaderBuffer;

static int Open(vlc_object_t* obj);
static void Close(vlc_object_t* obj);
static std::string trim(std::string &str_){
  str_.erase(str_.begin(), std::find_if(str_.begin(), str_.end(),
    std::not1(std::ptr_fun<int, int>(std::isspace))));
  str_.erase(std::find_if(str_.rbegin(), str_.rend(),
    std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str_.end());
  return str_;
}

vlc_module_begin()
	set_description("PPAPI HTTP/HTTPS Access Module")
	set_capability("access", 0)
	set_shortname("ppapi-remote-access")
	set_category(CAT_INPUT)
	set_subcategory(SUBCAT_INPUT_ACCESS)
        add_shortcut("http", "https", "ftp")
	set_callbacks(Open, Close)
vlc_module_end()
class TdDownloader {
public:
  int init(access_sys_t *sys);
  int read(uint8_t *dest, size_t num);

protected:
  int discover_request();
  int initDownload();
  int init_request_response(PP_Resource &request, PP_Resource &response, PP_Resource &loader, const std::string &method, const std::string &str_header);
  void parserHeader(const std::string &str_header);
  void addHeader(const std::string &str_header);

public:
  uint64_t fileSize;
  std::string contentType;
  bool canSeek;
  std::string url;
  std::unordered_map<std::string, std::string> headers;
  PP_Resource request_, response_, loader_;
  access_sys_t *sys;
};
struct access_sys_t {
  PP_Instance instance;
  PP_Resource message_loop;
  TdDownloader *downloader = nullptr;
  access_t* access = nullptr;
  void release(){
    if (downloader)
      delete downloader;
  }
};
/**
 * Class Downloader 
 * 
 **/
int TdDownloader::init(access_sys_t *sys)
{
  this->sys = sys;
  // Init CORS
  int ret = discover_request();
  if (ret == VLC_SUCCESS)
    ret = initDownload();
  return ret;
}
int TdDownloader::read(uint8_t *dest, size_t num)
{
  if (request_ == 0 || response_ == 0 || loader_ == 0)
    return -1;
  const vlc_ppapi_url_loader_t* iloader = vlc_getPPAPI_URLLoader();
  // int byteRead = iloader->ReadResponseBody(loader_, dest, num, PP_BlockUntilComplete());

  return 0;
}
int TdDownloader::init_request_response(PP_Resource &request, PP_Resource &response, PP_Resource &loader, const std::string &method, const std::string &str_header)
{
  const vlc_ppapi_url_loader_t* iloader = vlc_getPPAPI_URLLoader();
  const vlc_ppapi_url_request_info_t* irequest = vlc_getPPAPI_URLRequestInfo();
  const vlc_ppapi_url_response_info_t* iresponse = vlc_getPPAPI_URLResponseInfo();
  const vlc_ppapi_var_t* ivar = vlc_getPPAPI_Var();

  PP_Var head_method = vlc_ppapi_cstr_to_var(method.c_str(), method.length());
  PP_Var status_code; PP_Var location;
  PP_Bool success; int32_t code;
  int ret = VLC_EGENERIC;
  do {
    request = irequest->Create(sys->instance);
    if(request == 0) break;

    loader = iloader->Create(sys->instance);
    if(loader == 0) break;

    msg_Err(sys->access, "SetProperty: PP_URLREQUESTPROPERTY_METHOD");
    success = irequest->SetProperty(request, PP_URLREQUESTPROPERTY_METHOD, head_method);
    if (success != PP_TRUE) break;

    msg_Err(sys->access, "SetProperty: PP_URLREQUESTPROPERTY_URL");
    location = vlc_ppapi_cstr_to_var(url.c_str(), strlen(url.c_str()));
    success = irequest->SetProperty(request, PP_URLREQUESTPROPERTY_URL, location);
    vlc_ppapi_deref_var(location);
    if(success != PP_TRUE) break;

    msg_Err(sys->access, "SetProperty: PP_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS");
    success = irequest->SetProperty(request, PP_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS , PP_MakeBool(PP_TRUE));
    if(success != PP_TRUE) break;
    if (str_header.length() > 0){
      msg_Err(sys->access, "SetProperty: PP_URLREQUESTPROPERTY_HEADERS");
      PP_Var v_header = vlc_ppapi_cstr_to_var(str_header.c_str(), strlen(str_header.c_str()));
      success = irequest->SetProperty(request, PP_URLREQUESTPROPERTY_HEADERS , v_header);
      vlc_ppapi_deref_var(v_header);
      if(success != PP_TRUE) break;    
    }

    msg_Err(sys->access, "iloader->Open");
    code = iloader->Open(loader, request, PP_BlockUntilComplete());
    if(code != PP_OK) break;

    msg_Err(sys->access, "iloader->GetResponseInfo");
    response = iloader->GetResponseInfo(loader);
    if(response == 0) break;

    msg_Err(sys->access, "iloader->PP_URLRESPONSEPROPERTY_STATUSCODE");
    status_code = iresponse->GetProperty(response, PP_URLRESPONSEPROPERTY_STATUSCODE);
    assert(status_code.type == PP_VARTYPE_INT32);
    code = status_code.value.as_int; 
    if(code != 200) {
      msg_Err(sys->access, "couldn't retrieve file size: server returned `%i`", code);
      break;
    }
  }
  while (0);
  vlc_ppapi_deref_var(head_method);
  return VLC_SUCCESS;
}
int TdDownloader::initDownload()
{
  return init_request_response(request_, response_, loader_, "GET", "");;
}
int TdDownloader::discover_request()
{
  const vlc_ppapi_url_loader_t* iloader = vlc_getPPAPI_URLLoader();
  const vlc_ppapi_url_request_info_t* irequest = vlc_getPPAPI_URLRequestInfo();
  const vlc_ppapi_url_response_info_t* iresponse = vlc_getPPAPI_URLResponseInfo();
  const vlc_ppapi_var_t* ivar = vlc_getPPAPI_Var();

  PP_Resource request = 0, loader = 0, response = 0;
  PP_Var headers = PP_MakeUndefined();
  const char* header_str; size_t header_len;
  int ret = VLC_EGENERIC;
  do {
    ret = init_request_response(request, response, loader, "HEAD", "");
    if (ret != VLC_SUCCESS) break;
    
    headers = iresponse->GetProperty(response, PP_URLRESPONSEPROPERTY_HEADERS);
    assert(headers.type == PP_VARTYPE_STRING);
    header_str = ivar->VarToUtf8(headers, &header_len);
    // ParserHeader
    msg_Err(sys->access, "Reponse Header: %s", header_str);
    parserHeader(header_str);
    ret = VLC_SUCCESS;
  }
  while (0);
  if(request != 0) {
    vlc_subResReference(request);
  }
  if(response != 0) {
    vlc_subResReference(response);
  }
  if(loader != 0) {
    vlc_subResReference(loader);
  }
  vlc_ppapi_deref_var(headers);
  return ret;
}
void TdDownloader::parserHeader(const std::string &str_header)
{
  size_t i_from = 0; size_t i_found = 0;
  do {
    i_found = str_header.find("\r\n", i_from);
    if (i_found != std::string::npos){
      std::string header = str_header.substr(i_from, i_found);
      i_from = i_found + 2;
      addHeader(header);
    }
  }
  while(i_found != std::string::npos);
}
void TdDownloader::addHeader(const std::string &str_header)
{
  size_t i_found = str_header.find(":");
  if (i_found != std::string::npos){
    std::string field = str_header.substr(0, i_found);
    std::string value = str_header.substr(i_found + 1, str_header.length() - i_found - 1);
    headers[field] = trim(value);
  }
}

static ssize_t Read(access_t* access, uint8_t* dest, size_t bytes) {
  msg_Err(access, "Module Read: %i", bytes);
  access_sys_t* sys = (access_sys_t*)access->p_sys;
  return sys->downloader->read(dest, bytes);
}
static int Seek(access_t* access, uint64_t offset_from_start) {
  msg_Err(access, "Module Seek");
  return 0;
}
static int Control(access_t* access, int query, va_list args) {
  access_sys_t* sys = (access_sys_t*)access->p_sys;
  
  msg_Err(access, "Module Control");
  switch (query) {
  case ACCESS_CAN_SEEK:
    // XXX enabling this causes corruption:
    //case ACCESS_CAN_FASTSEEK:
  case ACCESS_CAN_CONTROL_PACE: // XXX required to be true.
  case ACCESS_CAN_PAUSE: {
    bool* ret = va_arg(args, bool*);
    *ret = sys->downloader->canSeek;
    break;
  }

  case ACCESS_CAN_FASTSEEK: {
    bool* ret = va_arg(args, bool*);
    *ret = false;
    break;
  }

  case ACCESS_GET_SIZE: {
    int64_t* size = va_arg(args, int64_t*);
    *size = (int64_t)sys->downloader->fileSize;
    break;
  }
  case ACCESS_GET_CONTENT_TYPE: {
    if(sys->downloader->contentType.empty()) { return VLC_EGENERIC; }

    char** dest = va_arg(args, char**);
    *dest = (char*)malloc(sys->downloader->contentType.size() + 1);
    memmove(*dest, sys->downloader->contentType.data(), sys->downloader->contentType.size());
    (*dest)[sys->downloader->contentType.size()] = '\0';
    break;
  }
  case ACCESS_SET_PAUSE_STATE: { break; }
  default:
    return VLC_EGENERIC;
  }

  return VLC_SUCCESS;
}
static int initIntance(access_t *access, PP_Instance &instance){
  vlc_value_t val;
  if(var_GetChecked(access->obj.libvlc, "ppapi-instance", VLC_VAR_INTEGER, &val) != VLC_ENOVAR) {
    instance = (PP_Instance)val.i_int;
  }
  if(instance == 0) {
    msg_Err(access, "couldn't get a reference to the PPAPI instance");
    return VLC_EGENERIC;
  }
  return VLC_SUCCESS;
}

static int initMsgLoop(access_t* access, const PP_Instance &instance, PP_Resource &msg_loop){
  const vlc_ppapi_message_loop_t* imsg_loop = vlc_getPPAPI_MessageLoop();
  msg_loop = imsg_loop->Create(instance);
  if(msg_loop == 0) {
    msg_Err(access, "failed to create message loop");
    return VLC_EGENERIC;
  }
  const int32_t code = imsg_loop->AttachToCurrentThread(msg_loop);
  if(code != PP_OK) {
    msg_Err(access, "error attaching thread to message loop: `%d`", code);
    return VLC_EGENERIC;
  }
  return VLC_SUCCESS;
}

static std::string realPath(access_t* access){
  char* c_str = NULL;
  asprintf(&c_str, "%s://%s",
          access->psz_access,
          access->psz_location);
  std::string location_str = std::move(std::string(c_str));
  free(c_str);
  return location_str;
}

static int Open(vlc_object_t* obj) {
  access_t* access = (access_t*)obj;

  // we can be openned at any time, so we must search up
  PP_Instance instance = 0;
  PP_Resource msg_loop = 0;
  std::string location_str;
  int ret = 0;
  do {
    ret = initIntance(access, instance);
    if (ret != VLC_SUCCESS)
      break;

    std::unique_ptr<access_sys_t> sys(new (std::nothrow) access_sys_t);
    ret = sys == nullptr ? VLC_ENOMEM : VLC_SUCCESS;
    if (ret != VLC_SUCCESS)
      break;
    ret = initMsgLoop(access, instance, msg_loop);
    if (ret != VLC_SUCCESS)
      break;
    msg_Err(access, "Init Done");
    location_str = realPath(access);
    sys->instance = instance;
    sys->message_loop = msg_loop;  
    sys->access = access;
    
    sys->downloader = new TdDownloader();
    sys->downloader->url = location_str;
    ret = sys->downloader->init(sys.get());
    if (ret != VLC_SUCCESS){
      sys->release();
      delete sys.get();
      break;
    }

    msg_Err(access, "Init Downloader");
    access_InitFields(access);
    access->pf_read = (Read);
    access->pf_seek = (Seek);
    access->pf_control = (Control);
    access->pf_block = NULL;
    access->p_sys = sys.release();
  }
  while (0);
  if (ret != VLC_SUCCESS && msg_loop != 0){
    // Release Msg Loop
    vlc_subResReference(msg_loop);
  }
  return ret;
}
static void Close(vlc_object_t* obj) {
  access_t* access = (access_t*)obj;
  access_sys_t* sys = access->p_sys;

  if(sys->message_loop != 0) {
    vlc_getPPAPI_MessageLoop()->PostQuit(sys->message_loop, PP_TRUE);
    vlc_subResReference(sys->message_loop);
  }
  // Free downloader
  sys->release();
  delete sys;
  return;
}
