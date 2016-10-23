# include <Siv3D.hpp>

double Synth(double t, double f)
{
	const double a = Sin(t * 440 * f * TwoPi);
	const double b = Sin(t * 440 * f * TwoPi * 2.0)*0.3;
	const double c = Sin(t * 440 * f * TwoPi * 3.0)*0.8;
	const double d = (Fraction(t * 440 * f)*2.0 - 1.0)*0.2;
	return (a + b + c + d) / 4.0*(1.0 - t);
}

void Main()
{
	Window::Resize(1280, 480);

	Recorder recorder(0, 5s, RecordingFormat::S44100, true);

	if (!recorder.start())
	{
		return;
	}

	const int32 numOfKeys = 39;

	std::array<Sound, numOfKeys> sounds;

	for (auto i : step(sounds.size()))
	{
		auto function = std::bind(Synth, std::placeholders::_1, Exp2(i / 12.0));
		sounds[i] = Sound(Wave(1.0s, function));
	}

	const std::array<int32, numOfKeys> keyOffset =
	{
		0, 1, 2,
		4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 
		18, 19, 20, 21, 22, 24, 25, 26, 27, 28, 29, 30,
		32, 33, 34, 35, 36, 38, 39, 40, 41, 42, 43, 44,
	};

	std::array<double, numOfKeys> keyP = {};
	std::array<std::pair<int, double>, 370> sp;

	while (System::Update())
	{
		const auto fft = FFT::Analyze(recorder, FFTSampleLength::SL4K);

		for (int32 i = 40; i < 410; ++i)
		{
			sp[i - 40] = { i, fft.buffer[i] };
		}

		std::sort(sp.begin(), sp.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

		const double index = (sp[0].first *sp[0].second + sp[1].first * sp[1].second) / (sp[0].second + sp[1].second);

		const int32 t = static_cast<int32>(Round(12 * Log2(index * fft.resolution() / 440)));

		for (auto i : step(numOfKeys))
		{
			keyP[i] = Saturate(keyP[i] + ((i == t && sp[0].second >= 0.003) ? 0.2 : -0.2));
		}

		for (auto i : step(numOfKeys))
		{
			if (keyOffset[i] % 2 == 0)
			{
				Rect(keyOffset[i] / 2 * 56, 0, 54, 400).draw(Lerp(Palette::White, Palette::Pink, keyP[i]));
			}
		}

		for (auto i : step(numOfKeys))
		{
			if (keyOffset[i] % 2 == 1)
			{
				Rect(38 + keyOffset[i] / 2 * 56, 0, 36, 250).draw(Lerp(Palette::Black, Palette::Pink, keyP[i]));
			}
		}

		bool played = false;

		for (auto i : step(numOfKeys))
		{
			if (keyOffset[i] % 2 == 1)
			{
				if (Rect(38 + keyOffset[i] / 2 * 56, 0, 36, 250).leftClicked)
				{
					played = true;
					sounds[i].playMulti();
				}
			}
		}

		for (auto i : step(numOfKeys))
		{
			if (keyOffset[i] % 2 == 0)
			{
				if (!played && Rect(keyOffset[i] / 2 * 56, 0, 54, 400).leftClicked)
				{
					sounds[i].playMulti();
				}
			}
		}
	}
}
