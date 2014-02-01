//  aweTrack.h :: Sound mixing track
//  Copyright 2012 - 2013 Keigen Shu

#ifndef AWE_TRACK_H
#define AWE_TRACK_H

#include <mutex>
typedef std::lock_guard<std::mutex> MutexLockGuard;

#include <string>
#include <algorithm>
#include "aweDefine.h"
#include "aweBuffer.h"
#include "aweSource.h"
#include "Filters/Rack.h"

namespace awe {

/**
 * Sound mixer class.
 *
 * Used to manage and mix multiple sound sources into one output track.
 * Filters can be chained to the filter rack to apply post-mixing sound
 * effects.
 *
 * All tracks are double buffered; the internal inaccessible source
 * mixing pool is labelled P while the output pool is labelled O.
 *
 * Every track has two mutexes; one is used to lock the pool buffer,
 * source list and pool config and the other is used to to lock the
 * output buffer and filter rack.
 */
class Atrack : public Asource
{
    typedef Filter::AscRack     AscRack;

private:
    mutable std::mutex  mPmutex;    //! Track pool mutex
    mutable std::mutex  mOmutex;    //! Track output mutex

    std::string         mName;      //! Track label
    ArenderConfig       mPconfig;   //! Track render configuration

    AsourceSet  mPsources;  //! Sound sources to mix from
    AfBuffer    mPbuffer;   //! Mixing buffer
    AfBuffer    mObuffer;   //! Output buffer
    AscRack     mOfilter;   //! Post-mixing filter rack

    bool        mqActive;   //! Is this source active?

private:
    /** Pull source into pool buffer, without mutex lock. */
    void fpull(Asource* src);

    /** Pull assigned sources into pool buffer, without mutex lock. */
    void fpull();

    /** Flip pool buffer with output buffer, without mutex lock. */
    void fflip();

    /** Apply filter rack onto output buffer, without mutex lock. */
    void ffilter();

public:
    Atrack(
        const size_t &sampleRate,
        const size_t &frames,
        const std::string &name = "Unnamed Track"
    );

    /**
     * This call does nothing on a track object.
     *
     * Please track the source and filter objects by your own.
     */
    virtual void drop() {}

    virtual void make_active(void*)
    {
        MutexLockGuard p_lock(mPmutex);
        mqActive = !mPsources.empty();
    }

    virtual bool is_active() const
    {
        MutexLockGuard p_lock(mPmutex);
        return mqActive;
    }

    virtual void render(AfBuffer &targetBuffer, const ArenderConfig &targetConfig);

    inline const ArenderConfig& getConfig() const { return mPconfig; }
    inline       ArenderConfig  setConfig(const ArenderConfig &new_config)
    {
        MutexLockGuard p_lock(mPmutex);
        return new_config;
    }

    /**
     * Retrieves the track filter rack.
     *
     * You can obtain the mutex object controlling the rack through the
     * Atrack::getMutex() call.
     */
    inline AscRack& getRack() { return mOfilter; }

    inline const AsourceSet& getSources() const { return mPsources; }
    inline const AfBuffer  & getOutput () const { return mObuffer; }

    /**
     * Retrieves the output mutex object which controls the output
     * buffer and the rack.
     */
    inline std::mutex & getMutex() { return mOmutex; }

    /** @return number of active sources within the pooling list */
    inline size_t count_active_sources() const
    {
        MutexLockGuard p_lock(mPmutex);
        size_t count = 0;
        for(Asource const * src : mPsources)
        {
            if (src->is_active() == true)
                count++;
        }
        return count;
    }

    /** @return number of sources within the pooling list */
    inline size_t count_sources() const {
        MutexLockGuard p_lock(mPmutex);
        return mPsources.size();
    }

    /** Inserts a source into the pooling list */
    inline void attach_source(Asource* const src)
    {
        MutexLockGuard p_lock(mPmutex);
        if (mPsources.count(src) == 0)
            mPsources.insert(src);

        mqActive = true;
    }

    /**
     * Removes a source from the pooling list.
     * @return false if the no objects were deleted from the pooling list.
     */
    inline bool detach_source(Asource* const src)
    {
        MutexLockGuard p_lock(mPmutex);
        bool r = mPsources.erase(src) != 0;
        mqActive = !mPsources.empty();
        return r;
    }

    /** Pull assigned sources into pool buffer, with mutex lock. */
    inline void pull()
    {
        MutexLockGuard p_lock(mPmutex);
        fpull();
    }

    /** Pulls the sources pool buffer, with mutex lock. */
    inline void pull(Asource *src)
    {
        MutexLockGuard p_lock(mPmutex);
        fpull(src);
    }

    /** Flip pool buffer with output buffer, with mutex lock. */
    inline void flip()
    {
        // Lock both mutexes without deadlock.
        std::lock(mPmutex, mOmutex);

        MutexLockGuard o_lock(mOmutex, std::adopt_lock);
        {
            // Unlock pool mutex after flipping.
            MutexLockGuard p_lock(mPmutex, std::adopt_lock);
            fflip();
        }

        ffilter();
    }

    /** Pushes the output buffer into a queue buffer. */
    inline void push(AfFIFOBuffer &queue) const
    {
        MutexLockGuard o_lock(mOmutex);

        for(auto s : mObuffer)
            queue.push(s);
    }

};


}

#endif
