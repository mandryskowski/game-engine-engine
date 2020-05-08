#include "AudioEngine.h"

AudioEngine::AudioEngine() :
	Device(nullptr), Context(nullptr), ListenerTransformPtr(nullptr)
{
	Init();
}

void AudioEngine::Init()
{
	Device = alcOpenDevice(nullptr);
	if (!Device)
	{
		std::cerr << "ERROR! Couldn't configure audio device.\n";
		return;
	}

	Context = alcCreateContext(Device, nullptr);
	if (!Context)
	{
		std::cerr << "ERROR! Couldn't create an OpenAL context.\n";
		return;
	}

	ALCboolean current = alcMakeContextCurrent(Context);
	if (current != ALC_TRUE)
	{
		std::cerr << "ERROR! Couldn't make an OpenAL context current.\n";
		return;
	}

	alListenerfv(AL_POSITION, glm::value_ptr(glm::vec3(0.0f)));
	alListenerfv(AL_VELOCITY, glm::value_ptr(glm::vec3(0.0f)));
}

///////////////////////////////////////////
//////////////////PRIVATE//////////////////
///////////////////////////////////////////

SoundBuffer* AudioEngine::LoadALBuffer(ALenum format, ALvoid* data, ALsizei size, ALsizei freq, std::string path)
{
	SoundBuffer* buffer = new SoundBuffer;

	alGenBuffers(1, &buffer->ALIndex);
	alBufferData(buffer->ALIndex, format, data, size, freq);

	buffer->Path = path;

	return buffer;
}

SoundBuffer* AudioEngine::SearchForAlreadyLoadedBuffer(std::string path)
{
	auto bufferIt = std::find_if(ALBuffers.begin(), ALBuffers.end(), [path](SoundBuffer* buffer) { return (buffer->Path == path && !buffer->Path.empty()); });

	if (bufferIt != ALBuffers.end())
			return *bufferIt;

	return nullptr;
}

///////////////////////////////////////////
//////////////////PUBLIC///////////////////
///////////////////////////////////////////

void AudioEngine::SetListenerTransformPtr(Transform* transformPtr)
{
	ListenerTransformPtr = transformPtr;
}

SoundSourceComponent* AudioEngine::AddSource(SoundSourceComponent* source)
{
	Sources.push_back(source);
	return source;
}

SoundSourceComponent* AudioEngine::AddSource(std::string path, std::string name)
{
	Sources.push_back(new SoundSourceComponent(name, LoadBuffer(path)));
	return Sources.back();
}

SoundSourceComponent* AudioEngine::FindSource(std::string name)
{
	auto sourceIt = std::find_if(Sources.begin(), Sources.end(), [name](SoundSourceComponent* source) { return source->GetName() == name; });

	if (sourceIt != Sources.end())
		return *sourceIt;

	std::cerr << "ERROR! Can't find sound source " + name + ".\n";
	return nullptr;
}

void AudioEngine::Update()
{
	////////////////// Update listener position & orientation
	alListenerfv(AL_POSITION, glm::value_ptr(ListenerTransformPtr->GetWorldTransform().PositionRef));

	glm::vec3 orientationVecs[2] = { ListenerTransformPtr->GetWorldTransform().FrontRef, glm::vec3(0.0f, 1.0f, 0.0f) };
	alListenerfv(AL_ORIENTATION, &orientationVecs[0].x);
}

SoundBuffer* AudioEngine::LoadBuffer(std::string path)
{
	////////////////// Check our current buffers; if one was loaded from the same path as passed to this function just return its address and don't waste time
	SoundBuffer* alreadyLoaded = SearchForAlreadyLoadedBuffer(path);
	if (alreadyLoaded)
		return alreadyLoaded;


	////////////////// If we have to load, call appropriate method based on path's file format
	std::string format = path.substr(path.find('.') + 1);
	SoundBuffer* buffer = nullptr;

	if (format == "wav")
		buffer = LoadBufferFromWav(path);
	else if (format == "ogg")
		buffer = LoadBufferFromOgg(path);
	else
	{
		std::cerr << "ERROR! Unrecognized audio format " << format << " of file " << path << '\n';
		return nullptr;
	}

	if (!buffer)
		return nullptr;

	ALBuffers.push_back(buffer);
	return ALBuffers.back();
}

SoundBuffer* AudioEngine::LoadBufferFromWav(std::string path)
{
	///////////////// Load the file
	AudioFile<float> file;

	if (!file.load(path))
	{
		std::cerr << "ERROR! Can't load WAV file " << path << "!\n";
		return nullptr;
	}

	///////////////// Assign a specific OpenAL format (and give a warning if the loaded type is stereo)
	bool stereo = file.isStereo();
	ALenum format = AL_FORMAT_MONO8;

	if (stereo)
		std::cerr << "INFO: Audio file " << path << " is stereo - it won't work with a 3D sound source.\n";

	if (file.getBitDepth() == 8)
		format = (stereo) ? (AL_FORMAT_STEREO8) : (AL_FORMAT_MONO8);
	else if (file.getBitDepth() == 16)
		format = (stereo) ? (AL_FORMAT_STEREO16) : (AL_FORMAT_MONO16);
	else
	{
		std::cerr << "ERROR! Unsupported bit depth of " << path << ". The bit depth count is: " << file.getBitDepth() << ".\n";
		return nullptr;
	}

	//////////////// Create an OpenAL buffer using our loaded data.
	std::vector <char> charSamples;
	//TODO: USE STL HERE!!!!
	for (unsigned int i = 0; i < file.samples[0].size(); i++)
	{
		charSamples.push_back((char)(file.samples[0][i] * 128.0f));
	}
	return LoadALBuffer(format, &charSamples[0], (ALsizei)charSamples.size(), (ALsizei)file.getSampleRate() / 2, path);
}

SoundBuffer* AudioEngine::LoadBufferFromOgg(std::string path)
{
	std::cerr << "Ogg Vorbis format is not currently supported.\n";
	return nullptr;
}

AudioEngine::~AudioEngine()
{
	if (Context)
		alcDestroyContext(Context);
}