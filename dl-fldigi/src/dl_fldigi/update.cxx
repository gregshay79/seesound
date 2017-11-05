/* 
 * Copyright (C) 2012 Daniel Richman
 * License: GNU GPL 3
 *
 * update.cxx: Automatically check for updates
 */

#include "dl_fldigi/update.h"

#include <stdexcept>
#include <map>
#include <string>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "config.h"
#include "debug.h"
#include "fl_digi.h"
#include "icons.h"

#include "jsoncpp.h"
#include "dl_fldigi/dl_fldigi.h"
#include "dl_fldigi/version.h"
#include "habitat/EZ.h"

using namespace std;

namespace dl_fldigi {
namespace update {

static UpdateThread *thr;
static bool update_new;
static string update_text, update_url;

static void update_check();
static void update_thread_done(void *what);
static void show_update();

/* check, cleanup are called by main thread only, while holding Fl lock */
void check()
{
    if (thr)
        return;

    LOG_INFO("checking for dl-fldigi updates");

    thr = new UpdateThread();
    thr->start();
}

void cleanup()
{
    while (thr)
        Fl::wait();
}

void *UpdateThread::run()
{
    update_check();
    Fl::awake(update_thread_done, this);
    return NULL;
}

/* invoked via Fl::awake; so we have the main Fl lock */
static void update_thread_done(void *what)
{
    if (what != thr)
    {
        LOG_ERROR("unknown thread");
        return;
    }

    if (update_new)
    {
        show_update();
        update_new = false;
    }

    LOG_INFO("cleaning up");
    thr->join();
    delete thr;
    thr = 0;
}

#ifdef __MINGW32__
#define PLATFORM "mingw32"
#endif

#ifdef __APPLE__
#define PLATFORM "macosx"
#endif

#ifdef __linux__
#define PLATFORM "linux"
#endif

#ifndef PLATFORM
#error "Couldn't work out what the platform should be for update checking :-("
#endif

static void update_check()
{
    map<string,string> args;
    args["platform"] = PLATFORM;
    args["commit"] = dl_fldigi::git_commit;

    string url = "http://habhub.org/dl-fldigi-check";
    url.append(EZ::cURL::query_string(args, true));

    EZ::cURL curl;
    string response;

    try
    {
        response = curl.get(url);
    }
    catch (runtime_error &e)
    {
        Fl_AutoLock lock;
        LOG_WARN("Error in update check: %s", e.what());
        return;
    }

    /* blocking download done, now get the lock: */
    Fl_AutoLock lock;

    if (response == "")
    {
        LOG_INFO("dl-fldigi is up to date");
        return;
    }

    Json::Reader reader;
    Json::Value val;

    if (!reader.parse(response, val, false) ||
        !val.isObject() || !val.size() ||
        !val["text"].isString() || !val["url"].isString())
    {
        LOG_WARN("Error in update check: Bad JSON");
        return;
    }

    update_text = val["text"].asString();
    update_url = val["url"].asString();

    // Strange bug causing empty dialog boxes and unresponse process
    // requires running got_update in the main thread, but whatever.
    update_new = true;
}

static void show_update()
{
    int c = fl_choice2("%s", "Close", "Open in browser", NULL,
                       update_text.c_str());
    if (c)
    {
        LOG_INFO("Opening %s in browser", update_url.c_str());
        // from fl_digi.h
        cb_mnuVisitURL(0, (void *) update_url.c_str());
    }
}

} /* namespace update */
} /* namespace dl_fldigi */
