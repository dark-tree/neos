
#include "types.h"


/**
 * @brief Sets the working area for the allocator and initializes the control structures.
 *
 * @note This one is quite slow, especially if size of the area is just a little bit larger, than a power of 2.
 *
 * @param[in] size Size of the allocator working area.
 * @param[in] offset Pointer to the first byte of the allocator working area.
 *
 * @return None.
 */
extern void kset(uint32_t size, void* offset);


/**
 * @brief This function finds a free memory area of a given size and allocates it.
 *
 * @note It will likely allocate a larger area, that the given one.
 *
 * @param[in] size Size of the desired memory area.
 *
 * @return Pointer to the allocated area if succeded, 0 if not.
 */
extern void* kmalloc(uint32_t size);


/**
 * @brief This function will free the given memory area (enabling it for further allocation).
 *
 * @param[in] pointer Pointer to anywhere within the previously allocated area.
 *
 * @return None.
 */
extern void kfree(void* pointer);


/**
 * @brief This function reserves (permamently takes out of the allocating pool) the given memory area at specified address.
 *
 * @note This one is quite slow too. It's recomended to run it right after kset on system startum.
 *       If you run in on an already allocated area - the behaviour is undefined (so run it before using kmalloc).
 *
 * @param[in] pointer Pointer to the first byte of the area you want to reserve.
 * @param[in] size Size of the memory area you want to reserve.
 *
 * @return None.
 */
extern void kmres(void* pointer, uint32_t size);


/**
 * @brief Takes an allocated area and makes it larger. If not possible, it runs kfree on that area and returns 0.
 * 
 * @note This function will try to enlarge the existing area if possible, if not - it will be moved and memory will be copied.
 *
 * @param[in] pointer Pointer to the FIRST BYTE of the area you want to enlarge.
 * @param[in] new_size Desired new size.
 *
 * @return Pointer to the enlarged memory area. 0 if enlargment was not possible.
 */
extern void* krealloc(void* pointer, uint32_t new_size);
