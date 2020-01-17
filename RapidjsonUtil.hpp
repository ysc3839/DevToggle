#pragma once

template <typename StringType>
class MyBasicStringWrapper {
public:
	typedef typename StringType::value_type Ch;

	MyBasicStringWrapper(StringType& string) : string_(string) {}

	void Put(Ch c) { string_.push_back(c); }
	void Flush() {}

	// Not implemented
	char Peek() const { RAPIDJSON_ASSERT(false); return 0; }
	char Take() { RAPIDJSON_ASSERT(false); return 0; }
	size_t Tell() const { RAPIDJSON_ASSERT(false); return 0; }
	char* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
	size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }

private:
	MyBasicStringWrapper(const MyBasicStringWrapper&);
	MyBasicStringWrapper& operator=(const MyBasicStringWrapper&);

	StringType& string_;
};

typedef MyBasicStringWrapper<std::string> MyStringWrapper;
typedef MyBasicStringWrapper<std::wstring> MyWStringWrapper;

bool UTF8CStrToUTF16WString(const char* src, std::wstring& dst)
{
	using rapidjson::UTF8;
	using rapidjson::UTF16;

	bool success = true;

	rapidjson::GenericStringStream<UTF8<>> source(src);
	MyWStringWrapper target(dst);

	while (source.Peek() != '\0')
	{
		if (!rapidjson::Transcoder<UTF8<>, UTF16<>>::Transcode(source, target))
		{
			success = false;
			break;
		}
	}

	return success;
}
