# DOOM From Scratch!

1. WAD File Parsing
    1. [WAD File Format and Data Types](#wad-file-format-and-data-types)
    2. [Loading the WAD File](#loading-the-wad-file)
    3. [Reader Functions for WAD Files](#reader-functions-for-wad-files)
    4. [Reading the Header](#reading-the-header)
    5. [Reading the Directory](#reading-the-directory)
    6. [Reading Palettes](#reading-palettes)
2. References

## WAD File Parsing

### WAD File Format and Data Types

DOOM WADs are binary files that are stored in Big Endian order, i.e., the most significant bytes are stored before lesser significant ones. In other words, the most significant byte is stored at the lowest memory address. This is important because modern computers do not use this kind of endianness but rather use Little Endian. This means that we must be careful when reading multibyte data from the WAD file so we need to reverse the byte order of each variable.

The DOOM WADs may contain only five different datatypes [1] which are summarized in the following table.

| Type | Bytes | Range |
|---|---|---|
| Unsigned Integer | 4 | [0, 4294967295]|
| Signed Integer | 4 | [-2147483648, 2147483647]|
| Unsigned Short | 2 | [0, 65535]|
| Signed Short | 2 | [-32768, 32767]|
| ASCII Character | 1 | [0, 255] |

### Loading the WAD File

The routine our WAD class constructor will invoke is the WAD loading one `void load_wad()`. This procedure will open an specified WAD file to be read in binary mode. The whole file will be read and stored in a class member `std::unique_ptr<uint8_t[]> m_wad_data`, i.e., a unique pointer to an array of bytes. Two design decisions were taken here: (1) we decided to use the `uint8_t` type, contained in the `cstdint` header due to its portability and implementation independence, and (2) we are using smart `std::unique_ptr` pointers to hold WAD data to make this data handling process less error prone providing that we will have an unique owner for the WAD data (the WAD class itself).

```c++
void load_wad(const std::string & filename)
{
	std::cout << "Reading WAD " << filename << "\n";
	std::ifstream wad_file_(filename, std::ios::binary | std::ios::ate);

	if (!wad_file_)
		throw std::runtime_error("Could not open file " + filename);

	std::streamsize wad_size_ = wad_file_.tellg();
	std::cout << "WAD file size is " << wad_size_ << "\n";

	m_wad_data = std::make_unique<uint8_t[]>((unsigned int)wad_size_);

	wad_file_.seekg(0, std::ios::beg);
	wad_file_.read((char*)m_wad_data.get(), wad_size_);
	wad_file_.close();

	std::cout << "WAD read successfully!\n";

	m_offset = 0;
}
```

### Reader Functions for WAD Files

```c++
short read_short(const std::unique_ptr<uint8_t[]> & rpData, unsigned int offset)
{
	short tmp_ = (rpData[offset + 1] << 8) | rpData[offset];
	return tmp_;
}

unsigned short read_ushort(const std::unique_ptr<uint8_t[]> rpData, unsigned int offset)
{
	unsigned short tmp_ = (rpData[offset + 1] << 8) | rpData[offset];
	return tmp_;
}
```

```c++
int read_int(const std::unique_ptr<uint8_t[]> & rpData, unsigned int offset)
{
	int tmp_ = (rpData[offset + 3] << 24) |
		(rpData[offset + 2] << 16)	|
		(rpData[offset + 1] << 8) |
		rpData[offset];

	return tmp_;
}

unsigned int read_uint(const std::unique_ptr<uint8_t[]> & rpData, unsigned int offset)
{
	unsigned int tmp_ = (rpData[offset + 3] << 24) |
		(rpData[offset + 2] << 16) |
		(rpData[offset + 1] << 8) |
		rpData[offset];

	return tmp_;
}
```

```c++
void copy_and_capitalize_buffer(
	std::string & rDst,
	const std::unique_ptr<uint8_t[]> & rpSrc,
	unsigned int offset,
	unsigned int srcLength)
{
	rDst = "";

	for (unsigned int i = 0; i < srcLength && rpSrc[offset + i] != 0; ++i)
		rDst += toupper(rpSrc[offset + i]);
}

```

### Reading the Header

The WAD header is the first thing that appears at the beginning of a WAD file. It consist of a sequence of 12 bytes divided into three 4-byte parts. Such parts are the following ones:

- (4 bytes) a not null-terminated ASCII string which indicates the type of the WAD file: either "IWAD" or "PWAD". The first one is an original file while the latter is a patched WAD file.
- (4 bytes) an unsigned integer that specifies the number of directory ENTRIES (total number of LUMPS) contained in the file.
- (4 bytes) an unsigned integer which holds the exact byte offset from the beginning of the WAD file where the DIRECTORY begins.

To store the WAD header, we declared a struct, namely WADHeader:

```c++
struct WADHeader
{
	std::string type;
	unsigned int lump_count;
	unsigned int directory_offset;
};

```

Then we added a member to our WAD class `WADHeader m_wad_header;` which we can fill with the header information coming from our WAD file thanks to the `read_header()` method. This procedure reads a 4-byte ASCII string into `m_wad_header.type`, a 4-byte unsigned integer into `m_wad_header.lump_count`, and another 4-byte unsigned integer into `m_wad_header.directory_offset`. 

```c++
#define WAD_HEADER_TYPE_LENGTH 4
#define WAD_HEADER_LUMPCOUNT_LENGTH 4
#define WAD_HEADER_OFFSET_LENGTH 4

[...]

void read_header()
{
	assert(m_wad_data);

	copy_and_capitalize_buffer(m_wad_header.type, m_wad_data, m_offset, WAD_HEADER_TYPE_LENGTH);
	m_offset += WAD_HEADER_TYPE_LENGTH;

	m_wad_header.lump_count = read_uint(m_wad_data, m_offset);
	m_offset += WAD_HEADER_LUMPCOUNT_LENGTH;

	m_wad_header.directory_offset = read_uint(m_wad_data, m_offset);
	m_offset += WAD_HEADER_OFFSET_LENGTH;
}
```

After reading the sample `doom1.wad` file, we can print the following header information:

```
WAD file
Type: IWAD
Lump Count: 1264
Directory Offset: 4175796
```

### Reading the Directory

```c++
struct WADEntry
{
	unsigned int offset;
	unsigned int size;
	std::string name;
};
```

```c++
void read_directory()
{
  assert(m_wad_data);
  assert(m_wad_header.lump_count != 0);

  m_offset = m_wad_header.directory_offset;
  for (unsigned int i = 0; i < m_wad_header.lump_count; ++i)
  {
    WADEntry entry_;
    entry_.offset = read_uint(m_wad_data, m_offset);
    m_offset += WAD_ENTRY_OFFSET_LENGTH;
    entry_.size = read_uint(m_wad_data, m_offset);
    m_offset += WAD_ENTRY_SIZE_LENGTH;
    copy_and_capitalize_buffer(entry_.name, m_wad_data, m_offset, 8);
    m_offset += WAD_ENTRY_NAME_LENGTH;

    m_directory.push_back(entry_);
    m_lump_map.insert(std::pair<std::string, unsigned int>(entry_.name, i));
  }
}
```

### Reading Paletes

```c++
struct WADPaletteColor
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
```

```c++
void read_palettes()
{
  assert(m_wad_data);
  assert(m_lump_map.find("PLAYPAL") != m_lump_map.end());

  WADEntry palettes_ = m_directory[m_lump_map["PLAYPAL"]];

  m_offset = palettes_.offset;
  while (m_offset < palettes_.offset + palettes_.size)
  {
    std::vector<WADPaletteColor> palette_(256);

    for (unsigned int i = 0; i < 256; ++i)
    {
      WADPaletteColor color_;
      color_.r = m_wad_data[m_offset++];
      color_.g = m_wad_data[m_offset++];
      color_.b = m_wad_data[m_offset++];
      palette_[i] = color_;
    }

    m_palettes.push_back(palette_);
  }
}
```

|   |   |   |   |
|---|---|---|---|
|![palette0](img/palette0.png)|![palette1](img/palette1.png)|![palette2](img/palette2.png)|![palette3](img/palette3.png)|
|![palette4](img/palette4.png)|![palette5](img/palette5.png)|![palette6](img/palette6.png)|![palette7](img/palette7.png)|
|![palette8](img/palette8.png)|![palette9](img/palette9.png)|![palette10](img/palette10.png)|![palette11](img/palette11.png)|
|![palette12](img/palette12.png)|![palette13](img/palette13.png)| | |

## References

- [1] [movax13h - Building a DOOM Engine from scratch with C/C++ and OpenGL – The WAD File – 001](http://www.movax13h.com/devlog/building-a-doom-engine-from-scratch-with-c-c-and-opengl-the-wad-file-001/)