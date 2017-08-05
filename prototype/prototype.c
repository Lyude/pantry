#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <unistd.h>

#include <pandriver.h>

/* If present, the prototype will use external precompiled shaders.
 * It is unclear if these shaders (generated by the proprietary
 * compiler) can be distributed. If they are not present, a dummy shader
 * will be used instead.
 *
 * TODO: Integrate with the assembler.
 *
 */

#ifdef PRECOMPILED_SHADERS
#include "../../shader_hex.h"
#else
uint32_t sample_fragment[16] = { 0x11 };
uint32_t sample_vertex[16] = { 0x11 };
#endif

int main()
{
	int fd = open_kernel_module();

	query_gpu_props(fd);

	init_cbma(fd);
	stream_create(fd, "malitl_339_0x53ae8");
	stream_create(fd, "malitl_339_0x53f78");

	uint64_t scratchpad = alloc_gpu_pages(fd, 8, 0xC);
	uint64_t heap_free_address = alloc_gpu_heap(fd, 0x8000);

	printf("Allocated what we need..\n");

	// size_t fb_size = 29 * 16 * 45 * 16 * 4 * 2;
	// uint64_t framebuffer = (uint64_t) (uint32_t) galloc(fb_size);

	/* Fake framebuffer to trap accesses */
	uint64_t framebuffer = 0x1CAFE0000;

	float vertices[] = {
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	printf("Tiling...\n");

	uint32_t tiler_jc =
		job_chain_vertex_tiler(fd,
			vertices, sizeof(vertices), CHAI_TRIANGLE_FAN,
			sample_vertex, sizeof(sample_vertex),
			sample_fragment, sizeof(sample_fragment),
			heap_free_address, scratchpad);

	uint32_t fragment_jc = job_chain_fragment(fd, framebuffer, heap_free_address, scratchpad);

	job_chain_replay(fd, tiler_jc, fragment_jc, heap_free_address, framebuffer);
	flush_job_queue(fd);

	sleep(3);

	/* Dump framebuffer to a file */
	/*uint8_t *fb = (uint8_t*) (uint32_t) framebuffer;
	FILE *fp = fopen("framebuffer.bin", "wb");
	fwrite(fb, 1, fb_size, fp);
	fclose(fp);*/

	/* Hang to prevent the tracer from going bananas */

	while(1);

	return 0;
}
