#include "base.h"
#include <stdlib.h>
//#include <stdio.h>


void DynamicArray::init(int _element_size_)
{
	num = 0;
	element_size = _element_size_;
	allocated = 0;
	data = NULL;
//	printf("init %d\n", element_size);
}

void DynamicArray::reserve(int size)
{
//	printf("        reserve  %d\n", size);
	if (allocated == 0){
		if (size > 0){
			allocated = size * element_size;
			data = malloc(allocated);
//			printf("          new  %p  ", data);
		}
	}else if (size * element_size > allocated){
		allocated = size * element_size * 2;
		void *data0 = data;
		data = realloc(data, allocated);
//		printf("          %p  ->  %p ", data0, data);
	}else if (size == 0)
		clear();
//	printf("        /r  %d\n", allocated);
}

void DynamicArray::resize(int size)
{
//	printf("        resize %d\n", size);
//	printf("        .\n");
	if (size > num){
		reserve(size);
		memset((char*)data + num * element_size, 0, (size - num) * element_size);
		/*for (int i=num;i<size;i++)
			init_sub_super_array(NULL, NULL, */
	}
	num = size;
//	printf("        /resize\n");
}

void DynamicArray::ensure_size(int size)
{
	if (size > num)
		resize(size);
}

void DynamicArray::insert_blank(int pos)
{
	resize(num + 1);
	if (num > pos + 1)
		memmove((char*)data + (pos + 1) * element_size,
		        (char*)data +  pos      * element_size,
		        (num - 1 - pos) * element_size);
	memset((char*)data + pos * element_size, 0, element_size);
}

void DynamicArray::append(const DynamicArray *a)
{
	if (a->num > 0){
//		printf("        append %d %d %d   - %d %d %d\n", num, element_size, allocated, a->num, a->element_size, a->allocated);
		int num_old = num;
		int num_a_old = a->num; // in case (this = a)
//		printf("        a\n");
		resize(num + a->num);
//		printf("        b\n");
		memcpy(&((char*)data)[num_old * element_size], a->data, num_a_old * element_size);
//		printf("        /append\n");
	}
}

void DynamicArray::append_8_single(int x, int y)
{
	reserve(num + 1);
	((int*)data)[num * 2] = x;
	((int*)data)[num * 2 + 1] = y;
	num ++;
}

void DynamicArray::append_4_single(int x)
{
	reserve(num + 1);
	((int*)data)[num ++] = x;
}

void DynamicArray::append_1_single(char x)
{
	reserve(num + 1);
	((char*)data)[num ++] = x;
}

void DynamicArray::append_single(const void *d)
{
	reserve(num + 1);
	memcpy(&((char*)data)[num * element_size], d, element_size);
	num ++;
}

void DynamicArray::insert_4_single(int x, int index)
{
	insert_blank(index);
	((int*)data)[index] = x;
}

void DynamicArray::insert_1_single(char x, int index)
{
	insert_blank(index);
	((char*)data)[index] = x;
}

void DynamicArray::insert_single(const void *d, int index)
{
	insert_blank(index);
	memcpy(&((char*)data)[index * element_size], d, element_size);
}

void DynamicArray::swap(int i1, int i2)
{
	if ((i1 < 0) || (i1 >= num))
		return;
	if ((i2 < 0) || (i2 >= num))
		return;
	if (i1 == i2)
		return;
	char *p1 = &((char*)data)[i1 * element_size];
	char *p2 = &((char*)data)[i2 * element_size];
	if (element_size == 1){
		char t = *p1;
		*p1 = *p2;
		*p2 = t;
	}else if (element_size == 4){
		int t = *(int*)p1;
		*(int*)p1 = *(int*)p2;
		*(int*)p2 = t;
	}else if (element_size <= 64){
		char t[64];
		memcpy(t, p1, element_size);
		memcpy(p1, p2, element_size);
		memcpy(p2, t, element_size);
	}else{
		char *t = new char[element_size];
		memcpy(t, p1, element_size);
		memcpy(p1, p2, element_size);
		memcpy(p2, t, element_size);
		delete[](t);
	}
}

void DynamicArray::reverse()
{
	int n = num / 2;
	for (int i=0;i<n;i++)
		swap(i, num - i - 1);
}

void DynamicArray::assign(const DynamicArray *a)
{
	if (a != this){
//		printf("assign\n");
		element_size = a->element_size;
		resize(a->num);
		if (num > 0)
			memcpy(data, a->data, num * element_size);
//		printf("/assign\n");
	}
}

void DynamicArray::exchange(DynamicArray &a)
{
	int tnum = num;
	num = a.num;
	a.num = tnum;
	void *tdata = data;
	data = a.data;
	a.data = tdata;
	int tall = allocated;
	allocated = a.allocated;
	a.allocated = tall;
}

void DynamicArray::clear()
{
	if (allocated > 0){
//		printf("        ~   %p %d  %d\n", data, element_size, num);
		free(data);
	}
	data = NULL;
	allocated = 0;
	num = 0;
}

void DynamicArray::delete_single(int index)
{
	int n = (num - 1) * element_size;
#if 0
	for (int i=index*element_size;i<n;i++)
	    ((char*)data)[i] = ((char*)data)[i + element_size];
#else
	memmove(&((char*)data)[index * element_size], &((char*)data)[(index + 1) * element_size], (num - index - 1) * element_size);
#endif
	num --;
}

void DynamicArray::delete_single_by_pointer(const void *p)
{	delete_single(index(p));	}

int DynamicArray::index(const void *p)
{	return ((long)p - (long)data) / element_size;	}

string dummy_summy;

bool DynamicArray::iterate(void *&p)
{
	if (p == NULL)
		p = data;
	else
		*(long*)&p += element_size;

	// still within list?
	//msg_write(format("%p  %p  %d  %d", p, data, element_size, num));
	dummy_summy = format("%p  %p  %d  %d", p, data, element_size, num);
	if ((long)p < (long)data + element_size * num)
		return true;
	//msg_write("f");

	// too far -> start at the beginning...
	p = data;
	return false;
}

bool DynamicArray::iterate_back(void *&p)
{
	if (p == NULL)
		p = (char*)data + (num - 1) * element_size;
	else
		*(long*)&p -= element_size;

	// still within list?
	if ((long)p >= (long)data)
		return true;

	// too far -> start at the ending...
	p = (char*)data + (num - 1) * element_size;
	return false;
}


DynamicArray DynamicArray::ref_subarray(int start, int num_elements)
{
	DynamicArray s;
	s.init(element_size);
	if (num_elements < 0)
		num_elements = start - num;
	if (num_elements > num - start)
		num_elements = num - start;
	s.num = num_elements;
	s.data = &((char*)data)[element_size * start];
	return s;
}



// Array<char>
template <> void Array<char>::add(char item)
{	((DynamicArray*)this)->append_1_single(item);	}
template <> void Array<char>::erase(int index)
{	((DynamicArray*)this)->delete_single(index);	}
template <> void Array<char>::operator = (const Array<char> &a)
{	((DynamicArray*)this)->assign(&a);	}
template <> void Array<char>::operator += (const Array<char> &a)
{	((DynamicArray*)this)->append(&a);	}

// Array<bool>
template <> void Array<bool>::add(bool item)
{	((DynamicArray*)this)->append_1_single(item);	}
template <> void Array<bool>::erase(int index)
{	((DynamicArray*)this)->delete_single(index);	}
template <> void Array<bool>::operator = (const Array<bool> &a)
{	((DynamicArray*)this)->assign(&a);	}
template <> void Array<bool>::operator += (const Array<bool> &a)
{	((DynamicArray*)this)->append(&a);	}

// Array<int>
template <> void Array<int>::add(int item)
{	((DynamicArray*)this)->append_4_single(item);	}
template <> void Array<int>::erase(int index)
{	((DynamicArray*)this)->delete_single(index);	}
template <> void Array<int>::operator = (const Array<int> &a)
{	((DynamicArray*)this)->assign(&a);	}
template <> void Array<int>::operator += (const Array<int> &a)
{	((DynamicArray*)this)->append(&a);	}

// Array<float>
template <> void Array<float>::add(float item)
{	((DynamicArray*)this)->append_4_single(*(int*)&item);	}
template <> void Array<float>::erase(int index)
{	((DynamicArray*)this)->delete_single(index);	}
template <> void Array<float>::operator = (const Array<float> &a)
{	((DynamicArray*)this)->assign(&a);	}
template <> void Array<float>::operator += (const Array<float> &a)
{	((DynamicArray*)this)->append(&a);	}



/*template <> void Array<char>::insert(char c, int pos)
{
	((DynamicArray*)this)->resize(((DynamicArray*)this)->num + 1);
	for (int i=((DynamicArray*)this)->num-2;i>=pos;i--)
		(*this)[i+1] = (*this)[i];
	(*this)[pos] = c;
}*/
