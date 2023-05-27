#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

struct i4004 {
	uint8_t rom[4096];
	uint8_t mem[256];
	uint8_t reg[16];
	uint16_t stack[3];
	unsigned int sptr;
	uint16_t cptr : 12;
	uint8_t dptr;
	uint8_t acc : 4;
	bool cy;
};

void spush(struct i4004* const this, const uint16_t cptr) {
	assert(0 <= this->sptr && this->sptr < 3);
	this->stack[this->sptr++] = cptr;
}

uint16_t spop(struct i4004* const this) {
	assert(0 < this->sptr);
	return this->stack[--this->sptr];
}

void ldm(struct i4004 *const this, const uint8_t imm) {
	this->acc = imm & 0xF;
}

void xch(struct i4004 *const this, const int index) {
	assert(0 <= index && index <= 15);
	const uint8_t t = this->reg[index] & 0xF;
	this->reg[index] = this->acc;
	this->acc = t;
}

void ld(struct i4004 *const this, const int index) {
	assert(0 <= index && index <= 15);
	this->acc = this->reg[index] & 0xF;
}

void clc(struct i4004* const this) {
	this->cy = false;
}

void stc(struct i4004* const this) {
	this->cy = true;
}

void clb(struct i4004* const this) {
	this->cy = false;
	this->acc = 0;
}

void fim(struct i4004* const this, const int index, const uint8_t imm) {
	assert(0 <= index && index <= 14);
	assert(index % 2 == 0);

	this->reg[index] = (imm & 0xF0) >> 4;
	this->reg[index + 1] = imm & 0xF;
}

void cmc(struct i4004* const this) {
	this->cy ^= true;
}

void cma(struct i4004* const this) {
	this->acc ^= 0xF;
}

void add(struct i4004* const this, const int index) {
	assert(0 <= index && index <= 15);
	const uint8_t result = this->acc + this->reg[index] + this->cy;
	this->cy = result > 0xF;
	this->acc = result & 0xF;
}

void sub(struct i4004* const this, const int index) {
	assert(0 <= index && index <= 15);
	const uint8_t result = this->acc + invert(this->reg[index]) + this->cy;
	this->cy = this->acc >= this->reg;
	this->acc = result & 0xF;
}

void iac(struct i4004* const this) {
	const uint8_t result = this->acc + 1;
	this->cy = result > 0xF;
	this->acc = result & 0xF;
}

void dac(struct i4004* const this) {
	const uint8_t result = this->acc - 1;
	this->cy = result < this->acc;
}

void inc(struct i4004* const this, const int index) {
	assert(0 <= index && index <= 15);
	++this->reg[index];
}

void ral(struct i4004* const this) {
	const uint8_t result = (this->acc << 1) | this->cy;
	this->cy = result & 0x10;
	this->acc = result & 0xF;
}

void rar(struct i4004* const this) {
	const uint8_t result = this->acc | (this->cy << 4);
	this->cy = result & 1;
	this->acc = result >> 1;
}

void tcc(struct i4004* const this) {
	this->acc = this->cy;
	this->cy = false;
}

void jun(struct i4004* const this, const uint16_t addr) {
	this->cptr = addr & 0xFFF;
}

void jcn_nc(struct i4004* const this, const uint16_t addr) {
	if (! this->cy)
		this->cptr = addr & 0xFFF;
}

void jcn_c(struct i4004* const this, const uint16_t addr) {
	if (this->cy)
		this->cptr = addr & 0xFFF;
}

void jcn_az(struct i4004* const this, const uint16_t addr) {
	if (this->acc == 0)
		this->cptr = addr & 0xFFF;
}

void jcn_an(struct i4004* const this, const uint16_t addr) {
	if (this->acc)
		this->cptr = addr & 0xFFF;
}

void isz(struct i4004* const this, const int index, const uint16_t addr) {
	assert(0 <= index && index <= 15);

	++this->reg[index];
	this->reg[index] &= 0xF;

	if (!this->reg[index])
		this->cptr = addr & 0xFFF;
}

// void jms(struct i4004* const this, )

void bbl(struct i4004* const this, const uint8_t val) {
	this->cptr = spop(this);
	this->acc = val & 0xF;
}

void src(struct i4004* const this, const int index) {
	assert(0 <= index && index <= 14);
	assert(index % 2 == 0);

	this->dptr = (this->reg[index] << 4)
		| this->reg[index + 1];
}

void wrm(struct i4004* const this) {
	assert(0 <= this->dptr && this->dptr <= 255);
	this->mem[this->dptr] = this->acc & 0xF;
}

void rdm(struct i4004* const this) {
	assert(0 <= this->dptr && this->dptr <= 255);
	this->acc = this->mem[this->dptr] & 0xF;
}

void fin(struct i4004* const this, const int index) {
	assert(0 <= index && index <= 14);
	assert(index % 2 == 0);
	const uint8_t ptr = (this->reg[0] << 4) | this->reg[1];
	const uint8_t imm = this->rom[ptr];
	this->reg[index] = (imm & 0xF0) >> 4;
	this->reg[index + 1] = imm & 0xF;
}

void adm(struct i4004* const this, const uint16_t address) {
	assert(address < sizeof(this->mem));
	const uint8_t result = this->acc + this->mem[address] + this->cy;
	this->cy = result > 0xF;
	this->acc = result & 0xF;
}

void sbm(struct i4004* const this, const uint16_t address) {
	assert(address < sizeof(this->mem));
	const uint8_t result
}

void execute(struct i4004* const chip) {
	while (true) {
		int clocks = 0;
		int skip = 1;
		const uint8_t op = chip->rom[chip->cptr];
		switch (op) {
			case 0x00 ... 0x0F: {
				clocks = 8;
			} break;

			case 0x10:
			case 0x12: {
				const uint8_t addr = chip->rom[chip->cptr + 1];
				jcn_c(chip, addr);
				++skip;
			} break;

			case 0x14: {
				const uint8_t addr = chip->rom[chip->cptr + 1];
				jcn_az(chip, addr);
				++skip;
			} break;

			case 0x1A: {
				const uint8_t addr = chip->rom[chip->cptr + 1];
				jcn_nc(chip, addr);
				++skip;
			} break;

			case 0x1C: {
				const uint8_t addr = chip->rom[chip->cptr + 1];
				jcn_an(chip, addr);
				++skip;
			} break;

			case 0x20 ... case 0x2F: {
				if (op % 2 == 0) {
					++skip;
					const uint8_t imm = chip->rom[chip->cptr + 1];
					fim(chip, op - 0x20, imm);
					clocks = 16;
				}
				else {
					src(chip, op - 0x21);
					clocks = 8;
				}
			} break;

			case 0x30 ... 0x3F: {
				if (op % 2 == 0) {
					fin(chip, op - 0x30);
					clocks = 16;
				}
				else {
					jin(chip, op - 0x31);
				}
			} break;

			case 0x40 ... 0x4F: {
				++skip;
				const uint8_t imm = chip->rom[chip->cptr + 1];
				jun(chip, op - 0x40, imm);
				clocks = 16;
			} break;

			case 0x50 ... 0x5F: {
				++skip;
				const uint8_t imm = chip->rom[chip->cptr + 1];
				jms(chip, op - 0x50, imm);
				clocks = 16;
			} break;

			case 0x60 ... 0x6F: {
				inc(chip, op - 0x60);
				clocks = 8;
			} break;

			case 0x70 ... 0x7F: {
				++skip;
				const uint8_t imm = chip->rom[chip->cptr + 1];
				isz(chip, op - 0x70, imm);
				clocks = 16;
			} break;

			case 0x80 ... 0x8F: {
				add(chip, op - 0x80);
				clocks = 8;
			} break;

			case 0x90 ... 0x9F: {
				sub(chip, op - 0x90);
				clocks = 8;
			} break;

			case 0xA0 ... 0xAF: {
				ld(chip, op - 0xA0);
				clocks = 8;
			} break;

			case 0xB0 ... 0xBF: {
				xch(chip, op - 0xB0);
				clocks = 8;
			} break;

			case 0xC0 ... 0xCF: {
				bbl(chip, op - 0xC0);
				clocks = 8;
				skip = 0;
			} break;

			case 0xD0 ... 0xDF: {
				ldm(chip, op - 0xD0);
				clocks = 8;
			} break;


			case 0xF0: {
				clb(chip);
				clocks = 8;
			} break;

			case 0xF1: {
				clc(chip);
				clocks = 8;
			} break;

			case 0xF2: {
				iac(chip);
				clocks = 8;
			} break;

			case 0xF3: {
				cmc(chip);
				clocks = 8;
			} break;

			case 0xF4: {
				cma(chip);
				clocks = 8;
			} break;

			case 0xF5: {
				ral(chip);
				clocks = 8;
			} break;

			case 0xF6: {
				rar(chip);
				clocks = 8;
			} break;

			case 0xF7: {
				tcc(chip);
				clocks = 8;
			} break;

			case 0xF8: {
				dac(chip);
				clocks = 8;
			} break;

			case 0xF9: {
				stc(chip);
				clocks = 8;
			} break;
		}
		chip->cptr += skip;
	}

}

int main(int argc, char* argv[]) {

}