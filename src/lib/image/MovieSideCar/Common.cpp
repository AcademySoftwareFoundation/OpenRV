//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <MovieSideCar/Common.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

namespace TwkMovie
{
    using namespace std;
    using namespace TwkFB;
    using namespace boost::algorithm;
    using namespace boost;

    namespace Message
    {

        const char* CommandPrefix() { return "@#"; }

        const size_t SizeInBytes() { return 4096; }

        const size_t MaxInQueue() { return 4; }

        const char* OK() { return "OK"; }

        const char* True() { return "True"; }

        const char* False() { return "False"; }

        const char* Failed() { return "failed"; }

        const char* Shutdown() { return "shutdown"; }

        const char* Acknowledge() { return "acknowledge"; }

        const char* ReadImage() { return "read_image"; }

        const char* ReadAudio() { return "read_audio"; }

        const char* AudioConfigure() { return "audio_configure"; }

        const char* CanConvertAudioRate() { return "can_convert_audio_rate"; }

        const char* OpenForRead() { return "open_for_read"; }

        const char* FileOpened() { return "file_opened"; }

        const char* GetInfo() { return "get_info"; }

        const char* ProtocolVersion() { return "protocol_version"; }

        const char* TimeOut() { return "timeout"; }

        const char* Throw() { return "throw"; }

        const char* Flush() { return "flush"; }

        bool isCommand(const char* buffer)
        {
            return buffer && buffer[0] == CommandPrefix()[0]
                   && buffer[1] == CommandPrefix()[1];
        }

        StringPair parseMessage(const char* processName, ostream* log,
                                const string& msg)
        {
            if (!isCommand(msg.c_str()))
            {
                return StringPair("", "");
            }

            string::size_type t = msg.find('!');
            string cmd = t == string::npos ? msg : msg.substr(2, t - 2);
            string arg = t == string::npos ? string("") : msg.substr(t + 1);

            while (arg.size() > 0 && arg[arg.size() - 1] == '\n')
                arg.erase(arg.size() - 1);

            if (log)
            {
                //(*log) << "[" << processName << "]:  got CMD = \"" << cmd <<
                //"\", ARG = \"" << arg << "\"" << endl;
            }

            return make_pair(cmd, arg);
        }

        size_t newMessage(const char* processName, ostream* log,
                          const string& cmd, const string& arg, char* outBuffer)
        {
            if (log)
            {
                //(*log) << "[" << processName << "]: send CMD = \"" << cmd <<
                //"\", ARG = \"" << arg << "\"" << endl;
            }

            ostringstream str;
            str << Message::CommandPrefix() << cmd << "!" << arg << endl;
            string outstr = str.str();

            if (outstr.size() >= Message::SizeInBytes())
            {
                cout << "WARNING: truncating message in newMessage" << endl;
                outstr.erase(Message::SizeInBytes() - 1, outstr.size());
            }

            strcpy(outBuffer, outstr.c_str());

            return outstr.size() + 1;
        }

        void decodeStringVector(const string& buffer,
                                TwkFB::FBInfo::StringVector& v)
        {
            split(v, buffer, is_any_of(string(",")));
            if (v.size() == 1 && v[0] == "")
                v.clear();
        }

        string encodeStringVector(const TwkFB::FBInfo::StringVector& v)
        {
            ostringstream str;

            for (size_t i = 0; i < v.size(); i++)
            {
                if (i)
                    str << ",";
                str << v[i];
            }

            return str.str();
        }

        string encodeMovieReadRequest(const Movie::ReadRequest& request)
        {
            ostringstream str;

            str << request.frame << "|" << request.stereo << "|"
                << request.missing << "|" << encodeStringVector(request.views)
                << "|" << encodeStringVector(request.layers) << "|"
                << encodeStringVector(request.channels);

            return str.str();
        }

        void decodeMovieReadRequest(const std::string& buffer,
                                    Movie::ReadRequest& request)
        {
            vector<string> v;

            split(v, buffer, is_any_of(string("|")));

            istringstream iframe(v[0]);
            iframe >> request.frame;

            istringstream istereo(v[1]);
            istereo >> request.stereo;

            istringstream imissing(v[2]);
            imissing >> request.missing;

            decodeStringVector(v[3], request.views);
            decodeStringVector(v[4], request.layers);
            decodeStringVector(v[5], request.channels);
        }

    } // namespace Message

} // namespace TwkMovie
