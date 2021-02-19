class FileMap
{
    char const* m_begin;
    char const* m_end;

public:

    FileMap(FileMap const&) = delete;
    FileMap& operator=(FileMap const&) = delete;

    FileMap(FileMap&& other) throw() :
        m_begin(other.m_begin),
        m_end(other.m_end)
    {
        other.m_begin = nullptr;
        other.m_end = nullptr;
    }

    FileMap& operator=(FileMap&& other) throw()
    {
        if (this != &other)
        {
            m_begin = other.m_begin;
            m_end = other.m_end;

            other.m_begin = nullptr;
            other.m_end = nullptr;
        }

        return *this;
    }

    explicit FileMap(wchar_t const* filename) throw() :
        m_begin(0),
        m_end(0)
    {
        invalid_handle file
        (
            CreateFile(filename,
                GENERIC_READ,
                FILE_SHARE_READ,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr)
        );

        if (!file) return;

        null_handle map
        (
            CreateFileMapping(file.get(),
                nullptr,
                PAGE_READONLY,
                0, 0,
                nullptr)
        );

        if (!map) return;

        LARGE_INTEGER size = {};

        if (!GetFileSizeEx(file.get(), &size)) return;

        m_begin = static_cast<char*>(MapViewOfFile(map.get(),
            FILE_MAP_READ,
