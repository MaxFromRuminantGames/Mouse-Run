CFLAGS = -g -O2 -Wall -Wextra -pedantic
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.c
	gcc $(CFLAGS) -o VulkanTest main.c $(LDFLAGS)

.PHONY: test clear

test: VulkanTest
	./VulkanTest

clear: VulkanTest
	rm -f VulkanTest
