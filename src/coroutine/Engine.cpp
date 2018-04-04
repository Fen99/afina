#include <afina/coroutine/Engine.h>
#include "../core/Debug.h"

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace Afina {
namespace Coroutine {

Engine::~Engine()
{
	while (alive != nullptr)
	{
		if (cur_routine == alive) { cur_routine = nullptr; }
		context* next = alive->next;
		delete alive;
		alive = next;
	}

	if (cur_routine != nullptr) { delete cur_routine; }
	if (idle_ctx != nullptr)    { delete idle_ctx; }
}

void Engine::Store(context &ctx) {
	char current_stack_position = 0;
	ctx.Hight = &current_stack_position;

	ASSERT(ctx.Hight < ctx.Low && ctx.Low != nullptr);

	size_t stack_size = ctx.Low - ctx.Hight;
	char* stack = (char*) calloc(stack_size, sizeof(char));
	memcpy(stack, ctx.Hight, stack_size);
	
	ctx.RemoveStack();
	std::get<0>(ctx.Stack) = stack;
	std::get<1>(ctx.Stack) = stack_size;
}

void Engine::Restore(context &ctx) {
	ASSERT(std::get<0>(ctx.Stack) != nullptr);
	ASSERT(ctx.Hight != 0 && std::get<1>(ctx.Stack) != 0);

	_Rewind(ctx);
}

void Engine::_Rewind(context &ctx) {
	char stack_marker = 0;
	if (&stack_marker >= ctx.Hight) { _Rewind(ctx); }
	else {
		memcpy(ctx.Hight, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack)); //Restore stack
		cur_routine = &ctx;
		longjmp(ctx.Environment, 1);
	}
}

void Engine::yield() {
	if (alive == nullptr) { return; }
	if (alive == cur_routine && alive->next != nullptr) {
		sched(alive->next);
	}
	sched(alive);
}

void Engine::sched(void *routine_) {
	context& ctx = *(static_cast<context*>(routine_));
	if (cur_routine != idle_ctx) { //Function was called from scheduller
		Store(*cur_routine); //Because storing of idle_ctx presented in start function
		if (setjmp(cur_routine->Environment) > 0) { return; } //corutine continue
	}

	Restore(ctx);
}

} // namespace Coroutine
} // namespace Afina
