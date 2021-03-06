#include "../kaba.h"
#include "../asm/asm.h"
#include "../../file/file.h"
#include <stdio.h>

namespace Kaba{

//#define ScriptDebug


extern const Class *TypeDynamicArray;


bool next_extern = false;
bool next_const = false;


static Node* _transform_insert_before_ = nullptr;

Node *conv_cbr(SyntaxTree *ps, Node *c, Variable *var);

string Operator::sig() const
{
	return "(" + param_type_1->name + ") " + PrimitiveOperators[primitive_id].name + " (" + param_type_2->name + ")";
}


Node *SyntaxTree::cp_node(Node *c)
{
	Node *cmd = new Node(c->kind, c->link_no, c->type);
	cmd->set_num_params(c->params.num);
	for (int i=0;i<c->params.num;i++)
		if (c->params[i])
			cmd->set_param(i, cp_node(c->params[i]));
	if (c->instance)
		cmd->set_instance(cp_node(c->instance));
	return cmd;
}

const Class *SyntaxTree::make_class_func(Function *f)
{
	return TypeFunctionP;
	string params;
	for (int i=0; i<f->num_params; i++){
		if (i > 0)
			params += ",";
		params += f->literal_param_type[i]->name;
	}
	return make_class("func(" + params + ")->" + f->return_type->name, Class::Type::POINTER, config.pointer_size, 0, TypeVoid);

	return TypePointer;
}

Node *SyntaxTree::ref_node(Node *sub, const Class *override_type)
{
	const Class *t = override_type ? override_type : sub->type->get_pointer();

	if (sub->kind == KIND_CLASS){
		// Class pointer
		auto *c = add_constant(TypeClassP);
		c->as_int64() = (long long)(int_p)sub->as_class();
		return add_node_const(c);
	}else if (sub->kind == KIND_FUNCTION_NAME){
		// can't be const because the function might not be compiled yet!
		return new Node(KIND_FUNCTION_POINTER, sub->link_no, make_class_func(sub->as_func()));
		// could also happen later via transform()
	}

	Node *c = new Node(KIND_REFERENCE, 0, t);
	c->set_num_params(1);
	c->set_param(0, sub);
	return c;
}

Node *SyntaxTree::deref_node(Node *sub, const Class *override_type)
{
	Node *c = new Node(KIND_UNKNOWN, 0, TypeVoid);
	c->kind = KIND_DEREFERENCE;
	c->set_num_params(1);
	c->set_param(0, sub);
	if (override_type)
		c->type = override_type;
	else
		c->type = sub->type->parent;
	return c;
}

Node *SyntaxTree::shift_node(Node *sub, bool deref, int shift, const Class *type)
{
	Node *c = new Node(deref ? KIND_DEREF_ADDRESS_SHIFT : KIND_ADDRESS_SHIFT, shift, type);
	c->set_num_params(1);
	c->set_param(0, sub);
	return c;
}

Node *SyntaxTree::add_node_statement(int index)
{
	Node *c = new Node(KIND_STATEMENT, index, TypeVoid);

	c->instance = nullptr;
	c->set_num_params(Statements[index].num_params);

	return c;
}

// virtual call, if func is virtual
Node *SyntaxTree::add_node_member_call(ClassFunction *f, Node *inst, bool force_non_virtual)
{
	Node *c;
	if ((f->virtual_index >= 0) and (!force_non_virtual)){
		c = new Node(KIND_VIRTUAL_CALL, f->virtual_index, f->return_type);
	}else{
		c = add_node_call(f->func);

		// some classes share a function (for example @DynamicArray.__subarray__)
		c->type = f->return_type;
	}
	c->set_instance(inst);
	c->set_num_params(f->param_types.num);
	return c;
}

Node *SyntaxTree::add_node_call(Function *f)
{
	Node *c = new Node(KIND_FUNCTION_CALL, (int_p)f, f->return_type);
	c->set_num_params(f->num_params);
	return c;
}

Node *SyntaxTree::add_node_func_name(Function *f)
{
	return new Node(KIND_FUNCTION_NAME, (int_p)f, TypeFunction);
}


Node *SyntaxTree::add_node_operator(Node *p1, Node *p2, Operator *op)
{
	Node *cmd = new Node(KIND_OPERATOR, (int_p)op, op->return_type);
	bool unitary = ((op->param_type_1 == TypeVoid) or (op->param_type_2 == TypeVoid));
	cmd->set_num_params( unitary ? 1 : 2); // unary / binary
	cmd->set_param(0, p1);
	if (!unitary)
		cmd->set_param(1, p2);
	return cmd;
}

Node *SyntaxTree::add_node_operator_by_inline(Node *p1, Node *p2, int inline_index)
{
	for (auto *op: operators)
		if (op->f->inline_no == inline_index)
			return add_node_operator(p1, p2, op);

	do_error("operator inline index not found: " + i2s(inline_index));
	return nullptr;
}


Node *SyntaxTree::add_node_local_var(Variable *v)
{
	if (!v)
		script->do_error_internal("var = nil");
	return new Node(KIND_VAR_LOCAL, (int_p)v, v->type);
}

Node *SyntaxTree::add_node_parray(Node *p, Node *index, const Class *type)
{
	Node *cmd_el = new Node(KIND_POINTER_AS_ARRAY, 0, type);
	cmd_el->set_num_params(2);
	cmd_el->set_param(0, p);
	cmd_el->set_param(1, index);
	return cmd_el;
}

/*Node *SyntaxTree::add_node_block(Block *b)
{
	return new Node(KIND_BLOCK, (long long)(int_p)b, TypeVoid);
}*/

SyntaxTree::SyntaxTree(Script *_script) :
	root_of_all_evil("RootOfAllEvil", TypeVoid, this)
{
	root_of_all_evil.block = new Block(&root_of_all_evil, nullptr);

	flag_string_const_as_cstring = false;
	flag_immortal = false;
	cur_func = nullptr;
	script = _script;
	asm_meta_info = new Asm::MetaInfo;
	for_index_count = 0;
	Exp.cur_line = nullptr;
	parser_loop_depth = 0;

	// "include" default stuff
	for (Package &p: Packages)
		if (p.used_by_default)
			AddIncludeData(p.script);
}


void SyntaxTree::parse_buffer(const string &buffer, bool just_analyse)
{
	Exp.Analyse(this, buffer + string("\0", 1)); // compatibility... expected by lexical
	
	PreCompiler(just_analyse);

	parse();

	Exp.clear();

	if (config.verbose)
		Show("par:a");

	ConvertCallByReference();

	/*if (FlagShow)
		Show();*/

	SimplifyShiftDeref();
	SimplifyRefDeref();
	
	PreProcessor();

	if (config.verbose)
		Show("par:b");
}


// override_line is logical! not physical
void SyntaxTree::do_error(const string &str, int override_exp_no, int override_line)
{
	// what data do we have?
	int logical_line = Exp.get_line_no();
	int exp_no = Exp.cur_exp;
	int physical_line = 0;
	int pos = 0;
	string expr;

	// override?
	if (override_line >= 0){
		logical_line = override_line;
		exp_no = 0;
	}
	if (override_exp_no >= 0)
		exp_no = override_exp_no;

	// logical -> physical
	if ((logical_line >= 0) and (logical_line < Exp.line.num)){
		physical_line = Exp.line[logical_line].physical_line;
		pos = Exp.line[logical_line].exp[exp_no].pos;
		expr = Exp.line[logical_line].exp[exp_no].name;
	}

	throw Exception(str, expr, physical_line, pos, script);
}

void SyntaxTree::CreateAsmMetaInfo()
{
	asm_meta_info->global_var.clear();
	for (int i=0;i<root_of_all_evil.var.num;i++){
		Asm::GlobalVar v;
		v.name = root_of_all_evil.var[i]->name;
		v.size = root_of_all_evil.var[i]->type->size;
		v.pos = root_of_all_evil.var[i]->memory;
		asm_meta_info->global_var.add(v);
	}
}



Constant *SyntaxTree::add_constant(const Class *type)
{
	auto *c = new Constant(type, this);
	constants.add(c);
	return c;
}



Function *SyntaxTree::add_function(const string &name, const Class *type)
{
	Function *f = new Function(name, type, this);
	functions.add(f);
	f->block = new Block(f, nullptr);
	return f;
}



Node *SyntaxTree::add_node_const(Constant *c)
{
	return new Node(KIND_CONSTANT, (int_p)c, c->type);
}

int SyntaxTree::which_primitive_operator(const string &name)
{
	for (int i=0;i<NUM_PRIMITIVE_OPERATORS;i++)
		if (name == PrimitiveOperators[i].name)
			return i;
	return -1;
}

const Class *SyntaxTree::which_owned_class(const string &name)
{
	for (auto *c: classes)
		if (name == c->name)
			return c;
	return nullptr;
}

int SyntaxTree::which_statement(const string &name)
{
	for (int i=0;i<Statements.num;i++)
		if (name == Statements[i].name)
			return i;
	return -1;
}


// xxxxx FIXME
Node *exlink_make_var_local(SyntaxTree *ps, const Class *t, Variable *v)
{
	return new Node(KIND_VAR_LOCAL, (int_p)v, t);
}

Node *exlink_make_var_element(SyntaxTree *ps, Function *f, ClassElement &e)
{
	Node *self = ps->add_node_local_var(f->__get_var(IDENTIFIER_SELF));
	Node *link = new Node(KIND_DEREF_ADDRESS_SHIFT, e.offset, e.type);
	link->set_num_params(1);
	link->params[0] = self;
	return link;
}

Node *exlink_make_func_class(SyntaxTree *ps, Function *f, ClassFunction &cf)
{
	Node *link = new Node(KIND_FUNCTION_NAME, (int_p)cf.func, TypeFunction);
	/*if (cf.virtual_index >= 0){
		link = new Node(KIND_VIRTUAL_CALL, cf.virtual_index, cf.script, cf.return_type);
	}else{
		link = new Node(KIND_FUNCTION_CALL, (int_p)cf.func, cf.script, cf.return_type);
	}*/
	//link->set_num_params(cf.param_types.num);
	Node *self = ps->add_node_local_var(f->__get_var(IDENTIFIER_SELF));
	link->set_instance(self);
	return link;
}

Array<Node*> SyntaxTree::get_existence_shared(const string &name)
{
	Array<Node*> links;

	// global variables (=local variables in "RootOfAllEvil")
	for (Variable *v: root_of_all_evil.var)
		if (v->name == name)
			return {new Node(KIND_VAR_GLOBAL, (int_p)v, v->type)};

	// named constants
	for (Constant *c: constants)
		if (name == c->name)
			return {new Node(KIND_CONSTANT, (int_p)c, c->type)};

	// then the (real) functions
	for (Function *f: functions)
		if (f->name == name and !f->_class)
			links.add(new Node(KIND_FUNCTION_NAME, (int_p)f, TypeFunction));//f->literal_return_type);
	if (links.num > 0)
		return links;

	// types
	auto *c = which_owned_class(name);
	if (c)
		return {new Node(KIND_CLASS, (int_p)c, TypeClass)};

	// ...unknown
	return {};
}

Array<Node*> SyntaxTree::get_existence(const string &name, Block *block)
{
	Array<Node*> links;

	if (block){
		Function *f = block->function;

		// first test local variables
		auto *v = block->get_var(name);
		if (v)
			return {exlink_make_var_local(this, v->type, v)};
		if (f->_class){
			if ((name == IDENTIFIER_SUPER) and (f->_class->parent))
				return {exlink_make_var_local(this, f->_class->parent->get_pointer(), f->__get_var(IDENTIFIER_SELF))};
			// class elements (within a class function)
			for (ClassElement &e: f->_class->elements)
				if (e.name == name)
					return {exlink_make_var_element(this, f, e)};
			for (ClassFunction &cf: f->_class->functions)
				if (cf.name == name)
					return {exlink_make_func_class(this, f, cf)};
		}
	}

	// shared stuff (global variables, functions)
	links = get_existence_shared(name);
	if (links.num > 0)
		return links;

	// then the statements
	int w = which_statement(name);
	if (w >= 0){
		Node *n = new Node(KIND_STATEMENT, w, TypeVoid);
		n->set_num_params(Statements[w].num_params);
		return {n};
	}

	// operators
	w = which_primitive_operator(name);
	if (w >= 0)
		return {new Node(KIND_PRIMITIVE_OPERATOR, w, TypeUnknown)};

	// in include files (only global)...
	for (Script *i: includes)
		links.append(i->syntax->get_existence_shared(name));

	// ...unknown
	return links;
}

// expression naming a type
const Class *SyntaxTree::find_type_by_name(const string &name)
{
	for (auto *c: classes)
		if (name == c->name)
			return c;
	for (Script *inc: includes)
		for (auto *c: inc->syntax->classes)
			if (name == c->name)
				return c;
	return nullptr;
}

Class *SyntaxTree::create_new_class(const string &name, Class::Type type, int size, int array_size, const Class *sub)
{
	if (find_type_by_name(name))
		do_error("class already exists");

	Class *t = new Class(name, size, this, sub);
	t->type = type;
	t->array_length = max(array_size, 0);
	classes.add(t);
	if (t->is_super_array() or t->is_dict()){
		t->derive_from(TypeDynamicArray, false);
		t->parent = sub;
		AddMissingFunctionHeadersForClass(t);
	}else if (t->is_array()){
		AddMissingFunctionHeadersForClass(t);
	}
	return t;
}

const Class *SyntaxTree::make_class(const string &name, Class::Type type, int size, int array_size, const Class *sub)
{
	// check if it already exists
	auto *tt = find_type_by_name(name);
	if (tt)
		return tt;

	// add new class
	return create_new_class(name, type, size, array_size, sub);
}

const Class *SyntaxTree::make_class_super_array(const Class *element_type)
{
	string name_pre = element_type->name;
	return make_class(name_pre + "[]",
			Class::Type::SUPER_ARRAY, config.super_array_size, -1, element_type);
}

const Class *SyntaxTree::make_class_array(const Class *element_type, int num_elements)
{
	string name_pre = element_type->name;
	return make_class(name_pre + format("[%d]", num_elements),
			Class::Type::ARRAY, element_type->size * num_elements, num_elements, element_type);
}

const Class *SyntaxTree::make_class_dict(const Class *element_type)
{
	return make_class(element_type->name + "{}",
			Class::Type::DICT, config.super_array_size, 0, element_type);
}

Node *conv_cbr(SyntaxTree *ps, Node *c, Variable *var)
{
	// convert
	if ((c->kind == KIND_VAR_LOCAL) and (c->as_local() == var)){
		c->type = c->type->get_pointer();
		return ps->deref_node(c);
	}
	return c;
}

#if 0
void conv_return(SyntaxTree *ps, nodes *c)
{
	// recursion...
	for (int i=0;i<c->num_params;i++)
		conv_return(ps, c->params[i]);
	
	if ((c->kind == KIND_STATEMENT) and (c->link_no == COMMAND_RETURN)){
		msg_write("conv ret");
		ref_command_old(ps, c);
	}
}
#endif


Node *conv_calls(SyntaxTree *ps, Node *c, int tt)
{
	if ((c->kind == KIND_STATEMENT) and (c->link_no == STATEMENT_RETURN))
		if (c->params.num > 0){
			if ((c->params[0]->type->is_array()) /*or (c->Param[j]->Type->IsSuperArray)*/){
				c->set_param(0, ps->ref_node(c->params[0]));
			}
			return c;
		}

	if ((c->kind == KIND_FUNCTION_CALL) or (c->kind == KIND_VIRTUAL_CALL) or (c->kind == KIND_ARRAY_BUILDER) or (c->kind == KIND_CONSTRUCTOR_AS_FUNCTION)){

		// parameters: array/class as reference
		for (int j=0;j<c->params.num;j++)
			if (c->params[j]->type->uses_call_by_reference()){
				c->set_param(j, ps->ref_node(c->params[j]));
			}

		// return: array reference (-> dereference)
		if ((c->type->is_array()) /*or (c->Type->IsSuperArray)*/){
			c->type = c->type->get_pointer();
			return ps->deref_node(c);
			//deref_command_old(this, c);
		}
	}

	// special string / list operators
	if (c->kind == KIND_OPERATOR){
		// parameters: super array as reference
		for (int j=0;j<c->params.num;j++)
			if ((c->params[j]->type->is_array()) or (c->params[j]->type->is_super_array())){
				c->set_param(j, ps->ref_node(c->params[j]));
			}
  	}
	return c;
}


// remove &*x
Node *easyfy_ref_deref(SyntaxTree *ps, Node *c, int l)
{
	if (c->kind == KIND_REFERENCE){
		if (c->params[0]->kind == KIND_DEREFERENCE){
			// remove 2 knots...
			return c->params[0]->params[0];
		}
	}
	return c;
}

// remove (*x)[] and (*x).y
Node *easyfy_shift_deref(SyntaxTree *ps, Node *c, int l)
{
	if ((c->kind == KIND_ADDRESS_SHIFT) or (c->kind == KIND_ARRAY)){
		if (c->params[0]->kind == KIND_DEREFERENCE){
			// unify 2 knots (remove 1)
			Node *t = c->params[0]->params[0];
			c->kind = (c->kind == KIND_ADDRESS_SHIFT) ? KIND_DEREF_ADDRESS_SHIFT : KIND_POINTER_AS_ARRAY;
			c->set_param(0, t);
			return c;
		}
	}

	return c;
}


Node *convert_return_by_memory(SyntaxTree *ps, Node *n, Function *f)
{
	ps->script->cur_func = f;

	if ((n->kind != KIND_STATEMENT) or (n->link_no != STATEMENT_RETURN))
		return n;

	// convert into   *-return- = param
	Node *p_ret = nullptr;
	for (Variable *v: f->var)
		if (v->name == IDENTIFIER_RETURN_VAR){
			p_ret = ps->add_node_local_var(v);
		}
	if (!p_ret)
		ps->do_error("-return- not found...");
	Node *ret = ps->deref_node(p_ret);
	Node *cmd_assign = ps->link_operator(OPERATOR_ASSIGN, ret, n->params[0]);
	if (!cmd_assign)
		ps->do_error("no = operator for return from function found: " + f->long_name);
	_transform_insert_before_ = cmd_assign;

	n->set_num_params(0);
	return n;
}


// convert "source code"...
//    call by ref params:  array, super array, class
//    return by ref:       array
void SyntaxTree::ConvertCallByReference()
{
	if (config.verbose)
		msg_write("ConvertCallByReference");
	// convert functions
	for (Function *f: functions){
		
		// parameter: array/class as reference
		for (int j=0;j<f->num_params;j++)
			if (f->var[j]->type->uses_call_by_reference()){
				f->var[j]->type = f->var[j]->type->get_pointer();

				// internal usage...
				transform_block(f->block, [&](Node *n){ return conv_cbr(this, n, f->var[j]); });
			}

		// return: array as reference
#if 0
		if ((func->return_type->is_array) /*or (f->Type->IsSuperArray)*/){
			func->return_type = GetPointerType(func->return_type);
			/*for (int k=0;k<f->Block->Command.num;k++)
				conv_return(this, f->Block->Command[k]);*/
			// no need... return gets converted automatically (all calls...)
		}
#endif
	}

	// convert return...
	for (Function *f: functions)
		if (f->return_type->uses_return_by_memory())
			//convert_return_by_memory(this, f->block, f);
			transform_block(f->block, [&](Node *n){ return convert_return_by_memory(this, n, f); });

	// convert function calls
	transform([&](Node *n){ return conv_calls(this, n, 0); });
}


void SyntaxTree::SimplifyRefDeref()
{
	// remove &*
	transform([&](Node *n){ return easyfy_ref_deref(this, n, 0); });
}

void SyntaxTree::SimplifyShiftDeref()
{
	// remove &*
	transform([&](Node *n){ return easyfy_shift_deref(this, n, 0); });
}

int __get_pointer_add_int()
{
	if (config.abi == Asm::INSTRUCTION_SET_AMD64)
		return INLINE_INT64_ADD_INT;
	return INLINE_INT_ADD;
}

Node *SyntaxTree::BreakDownComplicatedCommand(Node *c)
{
	if (c->kind == KIND_ARRAY){

		auto *el_type = c->type;

// array el -> array
//          -> index
//
// * -> + -> & array
//        -> * -> size
//             -> index

		Node *c_index = c->params[1];
		// & array
		Node *c_ref_array = ref_node(c->params[0]);
		// create command for size constant
		Node *c_size = add_node_const(add_constant(TypeInt));
		c_size->as_const()->as_int() = el_type->size;
		// offset = size * index
		Node *c_offset = add_node_operator_by_inline(c_index, c_size, INLINE_INT_MULTIPLY);
		c_offset->type = TypeInt;//TypePointer;
		// address = &array + offset
		Node *c_address = add_node_operator_by_inline(c_ref_array, c_offset, __get_pointer_add_int());
		c_address->type = el_type->get_pointer();//TypePointer;
		// * address
		return deref_node(c_address);
	}else if (c->kind == KIND_POINTER_AS_ARRAY){

		auto *el_type = c->type;

// array el -> array_pointer
//          -> index
//
// * -> + -> array_pointer
//        -> * -> size
//             -> index

		Node *c_index = c->params[1];
		Node *c_ref_array = c->params[0];
		// create command for size constant
		Node *c_size = add_node_const(add_constant(TypeInt));
		c_size->as_const()->as_int() = el_type->size;
		// offset = size * index
		Node *c_offset = add_node_operator_by_inline(c_index, c_size, INLINE_INT_MULTIPLY);
		c_offset->type = TypeInt;
		// address = &array + offset
		Node *c_address = add_node_operator_by_inline(c_ref_array, c_offset, __get_pointer_add_int());
		c_address->type = el_type->get_pointer();//TypePointer;
		// * address
		return deref_node(c_address);
	}else if (c->kind == KIND_ADDRESS_SHIFT){

		auto *el_type = c->type;

// struct el -> struct
//           -> shift (LinkNr)
//
// * -> + -> & struct
//        -> shift

		// & struct
		Node *c_ref_struct = ref_node(c->params[0]);
		// create command for shift constant
		Node *c_shift = add_node_const(add_constant(TypeInt));
		c_shift->as_const()->as_int() = c->link_no;
		// address = &struct + shift
		Node *c_address = add_node_operator_by_inline(c_ref_struct, c_shift, __get_pointer_add_int());
		c_address->type = el_type->get_pointer();//TypePointer;
		// * address
		return deref_node(c_address);
	}else if (c->kind == KIND_DEREF_ADDRESS_SHIFT){

		auto *el_type = c->type;

// struct el -> struct_pointer
//           -> shift (LinkNr)
//
// * -> + -> struct_pointer
//        -> shift

		Node *c_ref_struct = c->params[0];
		// create command for shift constant
		Node *c_shift = add_node_const(add_constant(TypeInt));
		c_shift->as_const()->as_int() = c->link_no;
		// address = &struct + shift
		Node *c_address = add_node_operator_by_inline(c_ref_struct, c_shift, __get_pointer_add_int());
		c_address->type = el_type->get_pointer();//TypePointer;
		// * address
		return deref_node(c_address);
	}
	return c;
}

Node* SyntaxTree::transform_node(Node *n, std::function<Node*(Node*)> F)
{
	if (n->kind == KIND_BLOCK){
		transform_block(n->as_block(), F);
	}else{
		for (int i=0; i<n->params.num; i++)
			n->set_param(i, transform_node(n->params[i], F));


		if (n->instance)
			n->set_instance(transform_node(n->instance, F));
	}

	return F(n);
}


void SyntaxTree::transform_block(Block *block, std::function<Node*(Node*)> F)
{
	//foreachi (Node *n, block->nodes, i){
	for (int i=0; i<block->params.num; i++){
		block->params[i] = transform_node(block->params[i], F);
		if (_transform_insert_before_){
			if (config.verbose)
				msg_error("INSERT BEFORE...");
			block->params.insert(_transform_insert_before_, i);
			_transform_insert_before_ = nullptr;
			i ++;
		}
	}
}

// split arrays and address shifts into simpler commands...
void SyntaxTree::transform(std::function<Node*(Node*)> F)
{
	_transform_insert_before_ = nullptr;
	for (Function *f: functions){
		cur_func = f;
		transform_block(f->block, F);
	}
}

Node *conv_constr_func(SyntaxTree *ps, Node *n)
{
	if (n->kind == KIND_CONSTRUCTOR_AS_FUNCTION){
		if (config.verbose){
			msg_error("constr func....");
			n->show();
		}
		_transform_insert_before_ = ps->cp_node(n);
		_transform_insert_before_->kind = KIND_FUNCTION_CALL;
		_transform_insert_before_->type = TypeVoid;
		if (config.verbose)
			_transform_insert_before_->show();

		// n->instance should be a reference to local... FIXME
		return ps->cp_node(n->instance->params[0]);
	}
	return n;
}

// split arrays and address shifts into simpler commands...
void SyntaxTree::BreakDownComplicatedCommands()
{
	if (config.verbose){
		Show("break:a");
		msg_write("BreakDownComplicatedCommands");
	}
	transform([&](Node* n){ return BreakDownComplicatedCommand(n); });
	transform([&](Node* n){ return conv_constr_func(this, n); });
	if (config.verbose)
		Show("break:b");
}

Node* conv_func_inline(SyntaxTree *ps, Node *n)
{
	if (n->kind == KIND_FUNCTION_CALL){
		if (n->as_func()->inline_no > 0){
			n->kind = KIND_INLINE_CALL;
			return n;
		}
	}
	if (n->kind == KIND_OPERATOR){
		Operator *op = n->as_op();
		n->kind = KIND_INLINE_CALL; // FIXME
		n->link_no = (int_p)op->f;
		return n;
	}
	return n;
}

void SyntaxTree::MakeFunctionsInline()
{
	transform([&](Node* n){ return conv_func_inline(this, n); });
	if (config.verbose)
		Show("break:c");
}



void MapLVSX86Return(Function *f)
{
	if (f->return_type->uses_return_by_memory()){
		foreachi(Variable *v, f->var, i)
			if (v->name == IDENTIFIER_RETURN_VAR){
				v->_offset = f->_param_size;
				f->_param_size += 4;
			}
	}
}

void MapLVSX86Self(Function *f)
{
	if (f->_class){
		foreachi(Variable *v, f->var, i)
			if (v->name == IDENTIFIER_SELF){
				v->_offset = f->_param_size;
				f->_param_size += 4;
			}
	}
}

void SyntaxTree::MapLocalVariablesToStack()
{
	for (Function *f: functions){
		f->_param_size = 2 * config.pointer_size; // space for return value and eBP
		if (config.instruction_set == Asm::INSTRUCTION_SET_X86){
			f->_var_size = 0;

			if (config.abi == ABI_WINDOWS_32){
				// map "self" to the VERY first parameter
				MapLVSX86Self(f);

				// map "-return-" to the first parameter
				MapLVSX86Return(f);
			}else{
				// map "-return-" to the VERY first parameter
				MapLVSX86Return(f);

				// map "self" to the first parameter
				MapLVSX86Self(f);
			}

			foreachi(Variable *v, f->var, i){
				if ((f->_class) and (v->name == IDENTIFIER_SELF))
					continue;
				if (v->name == IDENTIFIER_RETURN_VAR)
					continue;
				int s = mem_align(v->type->size, 4);
				if (i < f->num_params){
					// parameters
					v->_offset = f->_param_size;
					f->_param_size += s;
				}else{
					// "real" local variables
					v->_offset = - f->_var_size - s;
					f->_var_size += s;
				}				
			}
		}else if (config.instruction_set == Asm::INSTRUCTION_SET_AMD64){
			f->_var_size = 0;
			
			foreachi(Variable *v, f->var, i){
				long long s = mem_align(v->type->size, 4);
				v->_offset = - f->_var_size - s;
				f->_var_size += s;
			}
		}else if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			f->_var_size = 0;

			foreachi(Variable *v, f->var, i){
				int s = mem_align(v->type->size, 4);
				v->_offset = f->_var_size + s;
				f->_var_size += s;
			}
		}
	}
}


// no included scripts may be deleted before us!!!
SyntaxTree::~SyntaxTree()
{
	// delete all types created by this script
	for (auto *t: classes)
		if (t->owner == this)
			delete(t);

	if (asm_meta_info)
		delete(asm_meta_info);

	for (auto *f: functions)
		delete(f);

	for (auto *c: constants)
		delete(c);
}

void SyntaxTree::Show(const string &stage)
{
	if (!config.allow_output_stage(stage))
		return;
	msg_write("--------- Syntax of " + script->filename + "  " + stage + " ---------");
	msg_right();
	for (auto *f: functions)
		if (!f->is_extern)
			f->show(stage);
	msg_left();
	msg_write("\n\n");
}

};
