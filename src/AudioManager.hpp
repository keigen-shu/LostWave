//  AudioManager.hpp :: Game audio management system
//  Copyright 2011 - 2014 Keigen Shu

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

#include "libawe/aweEngine.h"
#include "libawe/Sources/Sample.h"
#include "libawe/Sources/Track.h"

#include "__zzCore.hpp"

using Sample        = awe::Source::Asample;
using SampleMap     = std::map<ulong, Sample*>;

using Track         = awe::Source::Atrack;
using TrackMap      = std::map<uchar, Track*>;

using VoiceMap      = std::map<Sample*, Track*>;
using VoiceMapNode  = VoiceMap::value_type;

/**
 * Class managing the sequencing of sound for the game.
 *
 * Each chart has a sample map which is loaded and then swapped onto
 * this class. When a play function is called, a sample-to-track map
 * node is created which would be used by the renderer when the
 * system buffer has been depleted.
 */
class AudioManager : public awe::AEngine
{
private:
    std::mutex                  mMutex;         //!< Sample and thread map mutex
    std::vector< std::thread* > mThreads;       //!< Threads relying on this.
    ulong                       mUpdateCount;   //!< Update sync counter
    std::atomic_flag            mRunning;       //!< Thread continuation flag

    SampleMap       mSampleMap; //!< Maps a Chart specific sample ID to it's sample object.
    TrackMap        mTrackMap;  //!< Maps an ID to a track.
    VoiceMap        mVoiceMap;  //!< Maps a sample to it's render destination.

public:
    /**
     * Creates and initializes the game's audio system.
     */
    AudioManager(size_t frame_count = 4096, size_t sample_rate = 48000);
    virtual ~AudioManager();
    virtual bool update();

    inline ulong getUpdateCount() const { return mUpdateCount; }

    inline std::mutex       & getMutex  () { return mMutex; }
    inline std::atomic_flag & getRunning() { return mRunning; }

    inline SampleMap * getSampleMap() { return &mSampleMap; }
    inline TrackMap  * getTrackMap () { return &mTrackMap; }
    inline VoiceMap  * getVoiceMap () { return &mVoiceMap; }

    inline Sample* getSample(ulong index)
    {
        SampleMap::iterator i = mSampleMap.find(index);
        return (i != mSampleMap.end()) ? i->second : nullptr;
    }
    inline Track * getTrack (uchar index)
    {
        TrackMap ::iterator i = mTrackMap .find(index);
        return (i != mTrackMap .end()) ? i->second : nullptr;
    }
    void wipe_SampleMap(bool drop_data = true);
    void swap_SampleMap(SampleMap& new_map);

    bool play(ulong, uchar, float = 1.0f, float = 0.0f, bool = false);

    void attach_thread(std::thread* thread_ptr);
};


#endif
