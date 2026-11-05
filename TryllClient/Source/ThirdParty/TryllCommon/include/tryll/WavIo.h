// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace Tryll::WavIo {

/// WAVE sample encodings we can decode.
enum class WavEncoding
{
	Pcm16,    ///< WAVE_FORMAT_PCM, 16-bit signed integer.
	Float32,  ///< WAVE_FORMAT_IEEE_FLOAT, 32-bit float, nominally in [-1, 1].
};

/// The `fmt ` chunk facts plus the raw bytes of the `data` chunk.
struct WavData
{
	WavEncoding          encoding = WavEncoding::Pcm16;
	uint32_t             sampleRate = 0;
	uint16_t             channels = 1;
	std::vector<uint8_t> bytes;

	[[nodiscard]] size_t bytesPerSample() const noexcept
	{
		return encoding == WavEncoding::Float32 ? 4u : 2u;
	}

	[[nodiscard]] size_t frames() const noexcept
	{
		const size_t stride = bytesPerSample() * (channels == 0 ? 1u : channels);
		return stride == 0 ? 0 : bytes.size() / stride;
	}
};

/// Parse a RIFF/WAVE file: read the `fmt ` chunk properly and return the `data`
/// chunk bytes undecoded.
///
/// **The format field is honoured, not assumed.** An earlier version of this
/// header read `fmt ` for its sample rate and channel count and then discarded the
/// encoding, reinterpreting whatever the `data` chunk held as 16-bit PCM. Handed a
/// 32-bit float WAV — which is what the Chatterbox ONNX bundle ships as its
/// reference voice — that silently produced noise of twice the expected length,
/// and the failure surfaced far downstream as clipped, half-intelligible speech.
/// Rejecting or decoding is the only pair of acceptable behaviours; guessing is
/// not one of them.
///
/// @throws std::runtime_error on I/O failure, a non-RIFF/WAVE file, a missing
///         `fmt ` or `data` chunk, or an encoding we do not decode.
[[nodiscard]] inline WavData ReadWavData(const std::filesystem::path& path)
{
	std::ifstream f{path, std::ios::binary};
	if (!f)
		throw std::runtime_error("Cannot open WAV: " + path.string());

	auto rd4 = [&]() -> uint32_t {
		uint32_t v = 0; f.read(reinterpret_cast<char*>(&v), 4); return v;
	};
	auto rd2 = [&]() -> uint16_t {
		uint16_t v = 0; f.read(reinterpret_cast<char*>(&v), 2); return v;
	};

	if (rd4() != 0x46464952u) throw std::runtime_error("Not a RIFF file: " + path.string());
	rd4(); // total size
	if (rd4() != 0x45564157u) throw std::runtime_error("Not a WAVE file: " + path.string());

	WavData out;
	uint16_t audioFormat = 0;
	uint16_t bitsPerSample = 0;
	bool haveFmt = false;
	bool haveData = false;

	while (f)
	{
		const uint32_t id = rd4();
		const uint32_t sz = rd4();
		if (!f) break;

		if (id == 0x20746d66u) // "fmt "
		{
			if (sz < 16)
				throw std::runtime_error("WAV fmt chunk too small: " + path.string());
			audioFormat    = rd2();
			out.channels   = rd2();
			out.sampleRate = rd4();
			rd4();                    // byte rate
			rd2();                    // block align
			bitsPerSample  = rd2();
			if (sz > 16)
				f.seekg(sz - 16, std::ios::cur);
			haveFmt = true;
		}
		else if (id == 0x61746164u) // "data"
		{
			out.bytes.resize(sz);
			f.read(reinterpret_cast<char*>(out.bytes.data()),
				static_cast<std::streamsize>(sz));
			haveData = true;
			break;
		}
		else
		{
			// Skip anything else — 'fact', 'PEAK' and 'LIST' all appear ahead of
			// 'data' in real files. RIFF chunks are word-aligned, so an odd size
			// carries a pad byte; missing it desynchronises every later chunk.
			f.seekg(sz + (sz & 1u), std::ios::cur);
		}
	}

	if (!haveFmt)
		throw std::runtime_error("WAV has no fmt chunk: " + path.string());
	if (!haveData)
		throw std::runtime_error("WAV has no data chunk: " + path.string());

	// 0xFFFE is WAVE_FORMAT_EXTENSIBLE, whose real encoding lives in a sub-format
	// GUID we do not parse; the bit depth is a reliable enough proxy for the two
	// encodings we accept.
	const bool extensible = audioFormat == 0xFFFEu;
	if ((audioFormat == 1u || extensible) && bitsPerSample == 16u)
		out.encoding = WavEncoding::Pcm16;
	else if ((audioFormat == 3u || extensible) && bitsPerSample == 32u)
		out.encoding = WavEncoding::Float32;
	else
	{
		throw std::runtime_error(
			"Unsupported WAV encoding (audio_format=" + std::to_string(audioFormat) +
			", bits_per_sample=" + std::to_string(bitsPerSample) +
			"); only 16-bit PCM and 32-bit float are decoded: " + path.string());
	}

	if (out.channels == 0)
		out.channels = 1;
	return out;
}

/// Read a **16-bit PCM** WAV file. Returns interleaved int16 samples in the order
/// they appear in the data chunk (mono or multi-channel) and writes the file's
/// sample rate to `outSampleRate`, and its channel count to `outChannels` when
/// that pointer is non-null.
///
/// Throws — rather than returning nonsense — when the file is not 16-bit PCM. Use
/// `ReadWavMonoFloat` for anything that may be float-encoded.
inline std::vector<int16_t> ReadWav16(
	const std::filesystem::path& path,
	uint32_t&                    outSampleRate,
	uint16_t*                    outChannels = nullptr)
{
	const WavData wav = ReadWavData(path);
	if (wav.encoding != WavEncoding::Pcm16)
	{
		throw std::runtime_error(
			"ReadWav16 requires 16-bit PCM but got 32-bit float; use "
			"ReadWavMonoFloat: " + path.string());
	}

	std::vector<int16_t> samples(wav.bytes.size() / 2);
	std::memcpy(samples.data(), wav.bytes.data(), samples.size() * 2);
	outSampleRate = wav.sampleRate;
	if (outChannels)
		*outChannels = wav.channels;
	return samples;
}

/// Write 16-bit PCM samples (interleaved if multi-channel) as a RIFF/WAVE file.
/// Throws if the file cannot be opened for writing.
inline void WriteWav16(
	const std::filesystem::path& path,
	std::span<const int16_t>     pcm,
	uint32_t                     sampleRate,
	uint16_t                     channels = 1)
{
	std::ofstream f{path, std::ios::binary};
	if (!f)
		throw std::runtime_error("Cannot open for writing: " + path.string());

	const uint32_t dataSize  = static_cast<uint32_t>(pcm.size() * 2u);
	const uint32_t fileSize  = 36u + dataSize;
	const uint32_t byteRate  = sampleRate * channels * 2u;
	const uint16_t blockAlign = static_cast<uint16_t>(channels * 2u);
	const uint16_t audioFmt   = 1; // PCM
	const uint16_t bps        = 16;
	const uint32_t fmtSize    = 16u;

	f.write("RIFF", 4);
	f.write(reinterpret_cast<const char*>(&fileSize), 4);
	f.write("WAVE", 4);
	f.write("fmt ", 4);
	f.write(reinterpret_cast<const char*>(&fmtSize), 4);
	f.write(reinterpret_cast<const char*>(&audioFmt), 2);
	f.write(reinterpret_cast<const char*>(&channels), 2);
	f.write(reinterpret_cast<const char*>(&sampleRate), 4);
	f.write(reinterpret_cast<const char*>(&byteRate), 4);
	f.write(reinterpret_cast<const char*>(&blockAlign), 2);
	f.write(reinterpret_cast<const char*>(&bps), 2);
	f.write("data", 4);
	f.write(reinterpret_cast<const char*>(&dataSize), 4);
	f.write(reinterpret_cast<const char*>(pcm.data()),
		static_cast<std::streamsize>(dataSize));
}

/// Mono float32 audio and the rate it was sampled at.
struct MonoFloatWav
{
	std::vector<float> samples;
	uint32_t           sampleRate = 0;
};

/// Read a WAV as mono float32 in [-1, 1], decoding either 16-bit PCM or 32-bit
/// float. Multi-channel input is averaged down to one channel.
///
/// Lives beside ReadWav16 rather than in an engine because reference-audio loading
/// is not an ONNX Runtime concern, and because a second WAV reader in this repo
/// would be the third.
///
/// **Both encodings matter in practice.** Our own committed fixtures and the STT
/// input clips are 16-bit PCM, while the Chatterbox ONNX bundle ships its reference
/// voice as 32-bit float — so a reader that handles only one of them fails on real
/// input, and one that handles only one *silently* fails in the worst possible way.
///
/// @throws std::runtime_error on I/O errors or an encoding we do not decode.
[[nodiscard]] inline MonoFloatWav ReadWavMonoFloat(const std::filesystem::path& path)
{
	const WavData wav = ReadWavData(path);
	const size_t channels = wav.channels;
	const size_t frames = wav.frames();

	MonoFloatWav out;
	out.sampleRate = wav.sampleRate;
	out.samples.resize(frames);

	if (wav.encoding == WavEncoding::Float32)
	{
		// Already float in [-1, 1] nominally; some encoders exceed it slightly and
		// that is the producer's business, not ours to normalise away.
		std::vector<float> interleaved(wav.bytes.size() / 4);
		std::memcpy(interleaved.data(), wav.bytes.data(), interleaved.size() * 4);
		for (size_t frame = 0; frame < frames; ++frame)
		{
			float sum = 0.0f;
			for (size_t c = 0; c < channels; ++c)
				sum += interleaved[frame * channels + c];
			out.samples[frame] = sum / static_cast<float>(channels);
		}
		return out;
	}

	std::vector<int16_t> interleaved(wav.bytes.size() / 2);
	std::memcpy(interleaved.data(), wav.bytes.data(), interleaved.size() * 2);
	constexpr float kScale = 1.0f / 32768.0f;
	for (size_t frame = 0; frame < frames; ++frame)
	{
		float sum = 0.0f;
		for (size_t c = 0; c < channels; ++c)
			sum += static_cast<float>(interleaved[frame * channels + c]) * kScale;
		out.samples[frame] = sum / static_cast<float>(channels);
	}
	return out;
}

/// Convert float PCM to 16-bit, clamping rather than wrapping.
///
/// The clamp is the point: a sample of 1.5 truncated into an int16 wraps to a
/// large negative value, so an over-range decoder output turns into a loud click
/// instead of clipping.
[[nodiscard]] inline std::vector<int16_t> FloatToPcm16(std::span<const float> samples)
{
	std::vector<int16_t> pcm(samples.size());
	for (size_t i = 0; i < samples.size(); ++i)
	{
		const float scaled = samples[i] * 32767.0f;
		const float clamped = scaled < -32768.0f ? -32768.0f
			: (scaled > 32767.0f ? 32767.0f : scaled);
		// Round rather than truncate: truncation biases every sample toward zero,
		// which is a DC offset on asymmetric content.
		pcm[i] = static_cast<int16_t>(clamped >= 0.0f ? clamped + 0.5f : clamped - 0.5f);
	}
	return pcm;
}

/// Resample mono float audio by linear interpolation.
///
/// **Naive on purpose, and only for one job.** There is no anti-aliasing filter, so
/// downsampling folds content above the new Nyquist frequency back into the audible
/// band. That is acceptable for conditioning a TTS voice reference — a one-off
/// per-voice conversion of speech that is already band-limited — and it is not
/// acceptable for a live audio path.
///
/// The good resampler is `AudioResampler` (miniaudio, windowed sinc), but it lives
/// in `tryll_server_lib`, which is downstream of every engine, so an engine cannot
/// reach it. Rather than let each engine grow its own copy of this loop — there
/// were already two — it lives here once. If a second consumer needs quality,
/// move `AudioResampler` down to `tryll_common` instead of improving this.
[[nodiscard]] inline std::vector<float> ResampleLinear(
	std::span<const float> samples, uint32_t srcRate, uint32_t dstRate)
{
	if (srcRate == 0 || dstRate == 0 || samples.empty())
		return {samples.begin(), samples.end()};
	if (srcRate == dstRate)
		return {samples.begin(), samples.end()};  // bitwise identity, no interpolation drift

	const double ratio = static_cast<double>(dstRate) / static_cast<double>(srcRate);
	const size_t outCount =
		static_cast<size_t>(static_cast<double>(samples.size()) * ratio + 0.5);
	std::vector<float> out(outCount);
	for (size_t i = 0; i < outCount; ++i)
	{
		const double srcPos = static_cast<double>(i) / ratio;
		const size_t i0 = static_cast<size_t>(srcPos);
		const size_t i1 = i0 + 1 < samples.size() ? i0 + 1 : samples.size() - 1;
		const float  frac = static_cast<float>(srcPos - static_cast<double>(i0));
		const float  a = samples[i0 < samples.size() ? i0 : samples.size() - 1];
		out[i] = a + (samples[i1] - a) * frac;
	}
	return out;
}

/// Write mono float audio as a 16-bit PCM WAV.
inline void WriteWavMonoFloat(
	const std::filesystem::path& path,
	std::span<const float>       samples,
	uint32_t                     sampleRate)
{
	WriteWav16(path, FloatToPcm16(samples), sampleRate, 1);
}

} // namespace Tryll::WavIo
