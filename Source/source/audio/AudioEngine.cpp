#include <audio/AudioEngine.h>
#include <audio/AudioFile.h>

AudioEngine::AudioEngine(GameManager* gameHandle) :
	Device(nullptr), Context(nullptr), ListenerTransformPtr(nullptr), GameHandle(gameHandle)
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

	std::cout << "SPRAWDZAM 1\n";
	CheckError();
	alGenBuffers(1, &buffer->ALIndex);
	alBufferData(buffer->ALIndex, format, data, size - size % 4, freq);
	std::cout << "SPRAWDZAM 2\n";
	CheckError();

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

Transform* AudioEngine::GetListenerTransformPtr()
{
	return ListenerTransformPtr;
}

void AudioEngine::Update()
{
	////////////////// Update listener position & orientation
	if (!ListenerTransformPtr)
		return;

	alListenerfv(AL_POSITION, glm::value_ptr((glm::vec3)ListenerTransformPtr->GetWorldTransform().PositionRef));

	glm::vec3 orientationVecs[2] = { ListenerTransformPtr->GetWorldTransform().GetFrontVec(), glm::vec3(0.0f, 1.0f, 0.0f) };
	alListenerfv(AL_ORIENTATION, &orientationVecs[0].x);
}

SoundBuffer* AudioEngine::LoadBufferFromFile(std::string path)
{
	////////////////// Check our current buffers; if one was loaded from the same path as passed to this function just return its address and don't waste time
	SoundBuffer* alreadyLoaded = SearchForAlreadyLoadedBuffer(path);
	if (alreadyLoaded)
		return alreadyLoaded;


	////////////////// If we have to load, call the appropriate method based on path's file format
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
	charSamples.reserve(file.samples[0].size());

	std::transform(file.samples[0].begin(), file.samples[0].end(), std::back_inserter(charSamples), [path](const float& sample) { return static_cast<char>(sample * 128.0f); });

	std::cout << path << ": " << charSamples.size() << ",  samplerate: " << file.getSampleRate() / 2 << ",  bitdepth: " << file.getBitDepth() << ",  length[s]: " << file.getLengthInSeconds() << '\n';
	return LoadALBuffer(format, &charSamples[0], (ALsizei)charSamples.size(), (ALsizei)file.getSampleRate() / 2, path);
}

SoundBuffer* AudioEngine::LoadBufferFromOgg(std::string path)
{
	std::cerr << "Ogg Vorbis format is not currently supported.\n";
	return nullptr;
}

void AudioEngine::CheckError()
{
	ALenum error = alGetError();
	std::cout << "AL error check...\n";
	if (error == AL_INVALID_NAME)
		std::cout << "Invalid name\n";
	else if (error == AL_INVALID_ENUM)
		std::cout << "Invalid enum\n";
	else if (error == AL_INVALID_VALUE)
		std::cout << "Invalid value\n";
	else if (error == AL_INVALID_OPERATION)
		std::cout << "Invalid operation\n";
	else if (error == AL_OUT_OF_MEMORY)
		std::cout << "Out of memory\n";
	else if (error == AL_NO_ERROR)
		std::cout << "No error\n";
	else
		std::cout << "Unknown error\n";
}

AudioEngine::~AudioEngine()
{
	if (Context)
		alcDestroyContext(Context);
}