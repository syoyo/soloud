/*
SoLoud audio engine
Copyright (c) 2013-2018 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include "soloud.h"

namespace SoLoud
{
	QueueInstance::QueueInstance(Queue *aParent)
	{
		mParent = aParent;
		mFlags |= PROTECTED;
	}
	
	void QueueInstance::getAudio(float *aBuffer, unsigned int aSamples)
	{
		if (mParent->mCount == 0)
		{
			return;			
		}
		unsigned int copycount = aSamples;
		unsigned int copyofs = 0;
		while (copycount && mParent->mCount)
		{
			mParent->mSource[mParent->mReadIndex]->getAudio(aBuffer + copyofs, copycount);
			copycount = 0;
			if (mParent->mSource[mParent->mReadIndex]->hasEnded())
			{
				delete mParent->mSource[mParent->mReadIndex];
				mParent->mSource[mParent->mReadIndex] = 0;
				mParent->mReadIndex = (mParent->mReadIndex + 1) % SOLOUD_QUEUE_MAX;
				mParent->mCount--;
			}
		}
	}

	bool QueueInstance::hasEnded()
	{
		return mParent->mCount == 0;
	}

	QueueInstance::~QueueInstance()
	{
	}

	Queue::Queue()
	{
		mQueueHandle = 0;
		mInstance = 0;
		mReadIndex = 0;
		mWriteIndex = 0;
		mCount = 0;
	}
	
	QueueInstance * Queue::createInstance()
	{
		if (mInstance)
		{
			stop();
			mInstance = 0;
		}
		mInstance = new QueueInstance(this);
		return mInstance;
	}

	void Queue::findQueueHandle()
	{
		// Find the channel the queue is playing on to calculate handle..
		int i;
		for (i = 0; mQueueHandle == 0 && i < (signed)mSoloud->mHighestVoice; i++)
		{
			if (mSoloud->mVoice[i] == mInstance)
			{
				mQueueHandle = mSoloud->getHandleFromVoice(i);
			}
		}
	}

	result Queue::play(AudioSource &aSound)
	{
		if (!mSoloud)
		{
			return INVALID_PARAMETER;
		}
	
		findQueueHandle();

		if (mQueueHandle == 0)
			return INVALID_PARAMETER;

		if (mCount >= SOLOUD_QUEUE_MAX)
			return OUT_OF_MEMORY;

		if (!aSound.mAudioSourceID)
		{
			aSound.mAudioSourceID = mSoloud->mAudioSourceID;
			mSoloud->mAudioSourceID++;
		}

		SoLoud::AudioSourceInstance *instance = aSound.createInstance();
		instance->mAudioSourceID = aSound.mAudioSourceID;

		if (instance == 0)
		{
			return OUT_OF_MEMORY;
		}

		mSoloud->lockAudioMutex();
		mSource[mWriteIndex] = instance;
		mWriteIndex = (mWriteIndex + 1) % SOLOUD_QUEUE_MAX;
		mCount++;
		mSoloud->unlockAudioMutex();

		return SO_NO_ERROR;
	}


	unsigned int Queue::getQueueCount()
	{
		unsigned int count;
		mSoloud->lockAudioMutex();
		count = mCount;
		mSoloud->unlockAudioMutex();
		return count;
	}

	bool Queue::isCurrentlyPlaying(AudioSource &aSound)
	{
		if (mCount == 0 || aSound.mAudioSourceID == 0)
			return false;
		mSoloud->lockAudioMutex();
		bool res = mSource[mReadIndex]->mAudioSourceID == aSound.mAudioSourceID;
		mSoloud->unlockAudioMutex();
		return res;
	}

	result Queue::setParamsFromAudioSource(AudioSource &aSound)
	{
		mChannels = aSound.mChannels;
		mBaseSamplerate = aSound.mBaseSamplerate;

	    return SO_NO_ERROR;
	}
	
	result Queue::setParams(float aSamplerate, unsigned int aChannels)
	{
	    if (aChannels < 1 || aChannels > MAX_CHANNELS)
	        return INVALID_PARAMETER;
		mChannels = aChannels;
		mBaseSamplerate = aSamplerate;
	    return SO_NO_ERROR;
	}
};
