#include "app.h"

int main(void){
	app_init();

	while (1) {
		app_run();
	}
}

// cant build without
__attribute__((cmse_nonsecure_entry)) int secure_add_one(int value) {
	return value + 1;
}
