#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace Afina {
namespace Coroutine {

int Engine::_stack_direction = 0;

Engine::~Engine()
{
	while (alive != nullptr)
	{
		if (cur_routine == alive) { cur_routine = nullptr; }
		context* next = alive->next;
		delete alive;
		alive = next;
	}

	//delete nullptr; - no effect command
	/*if (cur_routine != nullptr) */ delete cur_routine;
	/*if (idle_ctx != nullptr)    */ delete idle_ctx;
}

void Engine::Store(context &ctx) {
	char current_stack_position = 0;
	ctx.SetEndAddress(&current_stack_position);

	ASSERT(ctx.Low() < ctx.Hight() && ctx.Low() != nullptr);

	size_t stack_size = ctx.Hight() - ctx.Low();
	char* stack = (char*) calloc(stack_size, sizeof(char));
	memcpy(stack, ctx.Low(), stack_size);
	
	ctx.RemoveStack();
	ctx.Stack = stack;
}

void Engine::Restore(context &ctx) {
	ASSERT(ctx.Stack != nullptr);
	ASSERT(ctx.Low() < ctx.Hight() && ctx.Low() != nullptr);

	_Rewind(ctx);
}

void Engine::_Rewind(context &ctx) {
	char stack_marker = 0;

	ASSERT(_stack_direction != 0);
	
	if (_stack_direction > 0) {
		if (&stack_marker >= ctx.Low()) { 
			_Rewind(ctx); 
			stack_marker = 1;
		}
	}
	else {
		if (&stack_marker <= ctx.Hight()) { 
			_Rewind(ctx); 
			stack_marker = 1;
		}
	}

	if (stack_marker == 0) { //rewind was finished
		memcpy(ctx.Low(), ctx.Stack, ctx.Hight() - ctx.Low()); //Restore stack
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
