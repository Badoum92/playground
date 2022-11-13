#include "exo/string.h"
#include "exo/macros/assert.h"
#include <cstring>
#include <utility>

static void default_construct(exo::String &string)
{
	std::memset(string.storage.stack.buffer, 0, sizeof(string.storage.stack.buffer));
	string.storage.stack.is_small = 1;
	string.storage.stack.length   = 0;
}

namespace exo
{
// -- Constructors

String::String() { default_construct(*this); }

String::~String()
{
	if (!this->storage.stack.is_small) {
		ASSERT(this->storage.heap.buffer);
		std::free(this->storage.heap.buffer);
	}

	default_construct(*this);
}

String::String(const char *c_string) : String{c_string, strlen(c_string)} {}

String::String(const char *c_string, usize length)
{
	// If the string + the NULL delimiter fits in the stack buffer, store it inline
	if (length + 1 <= sizeof(this->storage.stack.buffer)) {
		std::memcpy(this->storage.stack.buffer, c_string, length);
		this->storage.stack.buffer[length] = '\0';
		this->storage.stack.is_small       = 1;
		this->storage.stack.length         = (unsigned char)(length);
	} else {
		this->storage.heap.length   = u32(length);
		this->storage.heap.capacity = u32(length + 1);
		this->storage.heap.buffer   = std::malloc(length + 1);
		std::memcpy(this->storage.heap.buffer, c_string, length);
		static_cast<char *>(this->storage.heap.buffer)[length] = '\0';
	}
}

String::String(const String &other) { *this = other; }

String &String::operator=(const String &other)
{
	// When we already have an allocation, we need to free our buffer in two cases:
	// - The other buffer does not have an allocation, thus we can store its content inline
	// - The other buffer has an allocation, but it has more content that our allocation can store
	if (!this->storage.stack.is_small &&
		(other.storage.stack.is_small || this->storage.heap.capacity < other.storage.heap.capacity)) {
		ASSERT(this->storage.heap.buffer);
		std::free(this->storage.heap.buffer);
	}

	// When the other string is allocated, we need to allocate for ourselves in two cases:
	// - We don't have an allocation, thus we need a new buffer
	// - We have an allocation but it is to small to contain the other string
	if (!other.storage.stack.is_small &&
		(this->storage.stack.is_small || this->storage.heap.capacity < other.storage.heap.capacity)) {
		this->storage.heap.buffer = std::malloc(other.storage.heap.capacity);
	}

	// Copy the other string into this
	if (other.storage.stack.is_small) {
		std::memcpy(this, &other, sizeof(String));
	} else {
		this->storage.heap.capacity = other.storage.heap.capacity;
		this->storage.heap.length   = other.storage.heap.length;
		std::memcpy(this->storage.heap.buffer, other.storage.heap.buffer, other.storage.heap.capacity);
	}

	return *this;
}

String::String(String &&other) { *this = std::move(other); }
String &String::operator=(String &&other)
{
	std::memcpy(this, &other, sizeof(String));
	default_construct(other);
	return *this;
}

// -- Element access

char &String::operator[](usize i)
{
	if (this->storage.stack.is_small) {
		ASSERT(i <= this->storage.stack.length);
		return this->storage.stack.buffer[i];
	} else {
		ASSERT(i <= this->storage.heap.length);
		return static_cast<char *>(this->storage.heap.buffer)[i];
	}
}

const char &String::operator[](usize i) const
{
	if (this->storage.stack.is_small) {
		ASSERT(i <= this->storage.stack.length);
		return this->storage.stack.buffer[i];
	} else {
		ASSERT(i <= this->storage.heap.length);
		return static_cast<char *>(this->storage.heap.buffer)[i];
	}
}

char &String::back()
{
	if (this->storage.stack.is_small) {
		ASSERT(this->storage.stack.length - 1 < String::SSBO_CAPACITY);
		return this->storage.stack.buffer[this->storage.stack.length - 1];
	} else {
		ASSERT(this->storage.heap.length < this->storage.heap.capacity);
		return static_cast<char *>(this->storage.heap.buffer)[this->storage.heap.length - 1];
	}
}

// -- Operations

void String::push_back(char c)
{
	if (this->storage.stack.is_small) {
		if (this->storage.stack.length + 2 < String::SSBO_CAPACITY) {
			this->storage.stack.buffer[this->storage.stack.length++] = c;
			this->storage.stack.buffer[this->storage.stack.length]   = '\0';
		} else {
			u32   new_capacity = String::SSBO_CAPACITY * 2;
			auto *new_buffer   = std::malloc(new_capacity);
			u32   new_length   = this->storage.stack.length + 1;

			std::memcpy(new_buffer, this->storage.stack.buffer, this->storage.stack.length);

			auto *chars           = static_cast<char *>(new_buffer);
			chars[new_length - 1] = c;
			chars[new_length]     = '\0';

			this->storage.heap.capacity = new_capacity;
			this->storage.heap.length   = new_length;
			this->storage.heap.buffer   = new_buffer;
		}
	} else {
		if (this->storage.heap.length + 2 >= this->storage.heap.capacity) {
			auto new_capacity = 2 * this->storage.heap.capacity;
			ASSERT(new_capacity);
			auto *new_buffer = std::realloc(this->storage.heap.buffer, new_capacity);

			this->storage.heap.capacity = new_capacity;
			this->storage.heap.buffer   = new_buffer;
		}

		char *chars = static_cast<char *>(this->storage.heap.buffer);

		chars[this->storage.heap.length++] = c;
		chars[this->storage.heap.length]   = '\0';
	}
}

} // namespace exo
