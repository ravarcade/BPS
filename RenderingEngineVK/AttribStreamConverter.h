


struct VkStream
{
	VkFormat format;
	uint32_t stride;
	void *data;
	VkStream() = default;
	VkStream(VkFormat _format, uint32_t _stride, void *_data) : format(_format), stride(_stride), data(_data) {}
	VkStream(VkFormat _format, uint32_t _stride, uint32_t _data) : format(_format), stride(_stride), data(reinterpret_cast<void *>(static_cast<uint64_t>(_data))) {}
};

using BAMS::RENDERINENGINE::Stream;

class CAttribStreamConverter
{
private:
	const static size_t tmpBufferSize = 1024;

	float tmpBuffer[tmpBufferSize];

public:
	CAttribStreamConverter() {}

	void Convert(VkStream dst, VkStream src, uint32_t len);
	void Convert(Stream &dst, Stream &src, uint32_t len);

	void ConvertAttribStreamDescription(Stream &out, VkStream src);
	void ConvertAttribStreamDescription(VkStream &out, Stream src);
};