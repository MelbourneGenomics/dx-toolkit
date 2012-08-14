#include "chunk.h"

#include <stdexcept>
#include <fstream>
#include <sstream>

#include <curl/curl.h>
#include <boost/thread.hpp>

#include "dxjson/dxjson.h"
#include "dxcpp/dxcpp.h"

#include "log.h"

using namespace std;
using namespace dx;

void Chunk::read() {
  const int64_t len = end - start;
  data.clear();
  data.resize(len);
  ifstream in(localFile.c_str(), ifstream::in | ifstream::binary);
  in.seekg(start);
  in.read(&(data[0]), len);
  if (in) {
  } else {
    ostringstream msg;
    msg << "readData failed on chunk " << (*this);
    throw runtime_error(msg.str());
  }
}

void Chunk::compress() {
  // TODO: compress the data into a new buffer, then swap that with the
  // uncompressed data
}

void checkConfigCURLcode(CURLcode code) {
  if (code != 0) {
    ostringstream msg;
    msg << "An error occurred while configuring the HTTP request (" << curl_easy_strerror(code) << ")" << endl;
    throw runtime_error(msg.str());
  }
}

void checkPerformCURLcode(CURLcode code) {
  if (code != 0) {
    ostringstream msg;
    msg << "An error occurred while performing the HTTP request (" << curl_easy_strerror(code) << ")" << endl;
    throw runtime_error(msg.str());
  }
}

/*
 * This function is the callback invoked by libcurl when it needs more data
 * to send to the server (CURLOPT_READFUNCTION). userdata is a pointer to
 * the chunk; we copy at most size * nmemb bytes of its data into ptr and
 * return the amount of data copied.
 */
size_t curlReadFunction(void * ptr, size_t size, size_t nmemb, void * userdata) {
  Chunk * chunk = (Chunk *) userdata;
  int64_t bytesLeft = chunk->size() - chunk->uploadOffset;
  size_t bytesToCopy = min<size_t>(bytesLeft, size * nmemb);

  if (bytesToCopy > 0) {
    memcpy(ptr, &((chunk->data)[chunk->uploadOffset]), bytesToCopy);
    chunk->uploadOffset += bytesToCopy;
  }

  return bytesToCopy;
}

void Chunk::upload() {
  // TODO: get the upload URL for this chunk; upload the data
  string url = uploadURL();
  LOG << "Upload URL: " << url << endl;

  CURL * curl = curl_easy_init();
  if (curl == NULL) {
    throw runtime_error("An error occurred when initializing the HTTP connection");
  }

  checkConfigCURLcode(curl_easy_setopt(curl, CURLOPT_POST, 1));
  checkConfigCURLcode(curl_easy_setopt(curl, CURLOPT_URL, url.c_str()));
  checkConfigCURLcode(curl_easy_setopt(curl, CURLOPT_READFUNCTION, curlReadFunction));
  checkConfigCURLcode(curl_easy_setopt(curl, CURLOPT_READDATA, this));

  /*
   * Set the Content-Type header. Default to something generic for now, and
   * do something sophisticated (i.e., detect it) later.
   */
  struct curl_slist * slist = NULL;
  slist = curl_slist_append(slist, "Content-Type: application/octet-stream");
  /*
   * Set the Content-Length header.
   */
  {
    ostringstream clen;
    clen << "Content-Length: " << size();
    slist = curl_slist_append(slist, clen.str().c_str());
  }
  checkConfigCURLcode(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist));

  checkPerformCURLcode(curl_easy_perform(curl));

  long responseCode;
  checkPerformCURLcode(curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode));
  if ((responseCode < 200) && (responseCode >= 300)) {
    ostringstream msg;
    msg << "Request failed with HTTP status code " << responseCode;
    throw runtime_error(msg.str());
  }

  curl_slist_free_all(slist);
  curl_easy_cleanup(curl);
}

void Chunk::clear() {
  // A trick for forcing a vector's contents to be deallocated: swap the
  // memory from data into v; v will be destroyed when this function exits.
  vector<char> v;
  data.swap(v);
}

string Chunk::uploadURL() const {
  JSON params(JSON_OBJECT);
  params["index"] = index + 1;  // minimum part index is 1
  JSON result = fileUpload(fileID, params);
  return result["url"].get<string>();
}

/*
 * Logs a message about this chunk.
 */
void Chunk::log(const string &message) const {
  LOG << "Thread " << boost::this_thread::get_id() << ": " << "Chunk " << (*this) << ": " << message << endl;
}

int64_t Chunk::size() const {
  return end - start;
}

ostream &operator<<(ostream &out, const Chunk &chunk) {
  out << "[" << chunk.localFile << ":" << chunk.start << "-" << chunk.end
      << " -> " << chunk.fileID << "[" << chunk.index << "]"
      << ", tries=" << chunk.triesLeft << ", data_size=" << chunk.data.size()
      << "]";
  return out;
}
