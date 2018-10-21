// From https://raw.githubusercontent.com/GIANTCRAB/Javascript-Performance-Test/master/js-performance-test.js
'use strict';

// Bad code
function badPerson() {
	// More than 4 hidden classes created for this
	
	// Hidden class created for Person
	function Person(name, age) {
		this.name = name,
		this.age = age;
	}
	
	// User ID was never declared, new hidden class "Person" has to be declared
	var mary = new Person("mary", 22); 
	mary.userId = 1;
	mary.userId = 10;
	mary.userId = 1;
	
	// Person with UserID AND Blood type was never declared, new hidden class "Person" has to be declared
	var bryan = new Person("bryan", 22); 
	bryan.userId = 2;
	bryan.bloodType = "AB";
	
	// Person with Experience AND Blood type was never declared, new hidden class "Person" has to be declared
	var alice = new Person("alice", 22); 
	alice.bloodType = "AB";
	alice.experience = 2.58;
	
	// Person with UserID AND Experience was never declared, new hidden class "Person" has to be declared
	var john = new Person("john", 22); 
	john.userId = 4;
	john.experience = 2.58;
	
	var johnCena = new Person("john cena", 22); 
	johnCena.bloodType = "AB";
	johnCena.userId = 5;
	
	var johnCena2 = new Person("john cena 2", 22); 
	johnCena2.bloodType = "AB";
	johnCena2.userId = 6;
	johnCena2.experience = 2.58;
	
	var johnCena3 = new Person("john cena 3", 22); 
	johnCena3.bloodType = "AB";
	johnCena3.userId = 7;
	johnCena3.experience = 5.58;
	
	var johnCena4 = new Person("john cena 4", 22); 
	johnCena4.userId = 8;
	johnCena4.experience = 5.58;
	
	var johnCena5 = new Person("john cena 5", 22); 
	johnCena4.userId = 9;
	johnCena4.experience = 9.58;
	
	var johnCena6 = new Person("john cena 6", 22); 
	johnCena6.bloodType = "AB";
	johnCena6.userId = 10;
	johnCena6.experience = 5.58;
	
	var johnCena7 = new Person("john cena 7", 22); 
	johnCena7.userId = 11;
	johnCena7.experience = 7.58;
}

function badPerson2() {
	// Unpredictable data types
	function Person(name, age, userId) {
		this.name = name,
		this.age = age,
		this.userId = userId;
	}
	
	var johnCena = new Person("john cena", 22, 1); 
	
	var johnCena2 = new Person("john cena 2", "22", 2); 
	
	var johnCena3 = new Person("john cena 3", 22, 3.0); 
	
	var johnCena4 = new Person("john cena 4", 22, "4"); 
	
	var johnCena5 = new Person("john cena 5", 22.0, 5); 

	var johnCena6 = new Person("john cena 6", 22.0, "6"); 
}

function badPerson3() {
	// Changing of data types, unpredictable. Boxing and unboxing required
	function Person(name, age, userId) {
		this.name = name,
		this.age = age,
		this.userId = userId;
	}
	
	var johnCena = new Person("john cena", 22, 1); 
	
	for(var i = 0; i < 100; i++) {
		johnCena.name = 1;
		johnCena.name = "john cena";
		johnCena.age = 22.5;
		johnCena.age = "22";
		johnCena.age = 22;
		johnCena.userId = 1.0;
		johnCena.userId = "1";
		johnCena.userId = i;
	}
}

function badMath() {
	// Polymorphic  use of operations
	function add(x, y) {
		return x + y;
	}
	
	for(var i = 0; i < 100; i++) {
		add(15346899, 236324512);
		add(235176, 124351);
		add(360450, 372033);
		add(531802, 57400);
		add("14234 sdfsfg sgh", "2 sdfgsdfg234");
		add("2sdfsdf", "1sdfsdfsdf");
		add("ewdftgsdftwer", "sfdgsdfgkiu sxc");
		add(1.583324, 2.23456);
		add(2236.06784, 1.0790567);
		add(0.949397335833968, 0.0522764346833391);
	}
}

function badLetDeclaration() {
	
	function Person(name, age) {
		this.name = name,
		this.age = age,
		this.bloodType = "",
		this.userId = 0,
		this.experience = 0.0;
	}
	
	function doDeclaration(x) {
		var johnCena = i;
		var johnCena2 = "1";
		var johnCena3 = 1.0;
		var johnCena4 = new Date();
		var johnCena5 = 1 + i;
		var realJohnCena = new Person("John Cena", 22);
		var realJohnCena2 = new Person("John Cena", 22);
		var realJohnCena3 = new Person("John Cena", 22);
		
		realJohnCena.bloodType = "AB";
		realJohnCena.userId = x;
		realJohnCena2.bloodType = "AB";
		realJohnCena2.experience = 1.5 + x;
		realJohnCena3.bloodType = "AB";
	}
	
	for(var i = 0; i < 50; i++) {
		doDeclaration(i);
	}
}

function badArrayDeclaration() {
	// Array is sparse, first value declared with [5]. Array switched to dictionary mode, which slows it down by a lot
	function doDeclaration() {
		var johnCenas = new Array(500);
		
		johnCenas[5] = "And his name is John Cena!";
		johnCenas[85] = "And his name is John Cena 2!";
	}
	
	for(var i = 0; i < 50; i++) {
		doDeclaration();
	}
}

// Good code 
function goodPerson() {
	
	// Hidden class created for person
	function Person(name, age) {
		this.name = name,
		this.age = age,
		this.bloodType = "", // Blood type is declared
		this.userId = 0, // User id is declared
		this.experience = 0.0; // Experience is declared
	}

	var mary = new Person("mary", 22); 
	mary.userId = 1;
	mary.userId = 10;
	mary.userId = 1;
	
	var bryan = new Person("bryan", 22); 
	bryan.userId = 2;
	bryan.bloodType = "AB";
	
	var alice = new Person("alice", 22); 
	alice.bloodType = "AB";
	alice.experience = 2.58;
	
	var john = new Person("john", 22); 
	john.userId = 4;
	john.experience = 2.58;
	
	var johnCena = new Person("john cena", 22); 
	johnCena.bloodType = "AB";
	johnCena.userId = 5;
	
	var johnCena2 = new Person("john cena 2", 22); 
	johnCena2.bloodType = "AB";
	johnCena2.userId = 6;
	johnCena2.experience = 2.58;
	
	var johnCena3 = new Person("john cena 3", 22); 
	johnCena3.bloodType = "AB";
	johnCena3.userId = 7;
	johnCena3.experience = 5.58;
	
	var johnCena4 = new Person("john cena 4", 22); 
	johnCena4.userId = 8;
	johnCena4.experience = 5.58;
	
	var johnCena5 = new Person("john cena 5", 22); 
	johnCena4.userId = 9;
	johnCena4.experience = 9.58;
	
	var johnCena6 = new Person("john cena 6", 22); 
	johnCena6.bloodType = "AB";
	johnCena6.userId = 10;
	johnCena6.experience = 5.58;
	
	var johnCena7 = new Person("john cena 7", 22); 
	johnCena7.userId = 11;
	johnCena7.experience = 7.58;
}

function goodPerson2() {
	// Predictable data types
	function Person(name, age, userId) {
		this.name = name,
		this.age = age,
		this.userId = userId;
	}
	
	var johnCena = new Person("john cena", 22.0, 1.0); 
	
	var johnCena2 = new Person("john cena 2", 22.0, 2.0); 
	
	var johnCena3 = new Person("john cena 3", 22.0, 3.0); 
	
	var johnCena4 = new Person("john cena 4", 22.0, 4.0); 
	
	var johnCena5 = new Person("john cena 5", 22.0, 5.0); 

	var johnCena6 = new Person("john cena 6", 22.0, 6.0); 
}

function goodPerson3() {
	// No changing of data types, predictable. Boxing and unboxing NOT required
	function Person(name, age, userId) {
		this.name = name,
		this.age = age,
		this.userId = userId;
	}
	
	var johnCena = new Person("john cena", 22.0, 1.0); 
	for(var i = 0; i < 100; i++) {
		johnCena.name = "john cena";
		johnCena.name = "john cena";
		johnCena.age = 22.0;
		johnCena.age = 22.0;
		johnCena.age = 22.0;
		johnCena.userId = i;
		johnCena.userId = i + 1;
		johnCena.userId = i + 2;
	}
}

function es2015Person3() {
	// Strictly define the data types, available only in ES2015
	// You'll only see performance improvement for certain kinds of data, but not in this test scenario
	class Person { 
		constructor (buffer = new ArrayBuffer(24)) { 
			this.buffer = buffer; 
		}
		set buffer (buffer) { 
			this._buffer  = buffer; 
			this._userId  = new Uint32Array (this._buffer,  0,  1); 
			this._name  = new Uint8Array  (this._buffer,  4, 16); 
			this._age = new Float32Array(this._buffer, 20,  1); 
		}
		get buffer () { 
			return this._buffer; 
		} 
		set userId (v) { this._userId[0] = v; } 
		get userId () { return this._userId[0]; } 
		set name (v) { this._name[0] = v; } 
		get name () { return this._name[0]; } 
		set age (v) { this._age[0] = v; } 
		get age () { return this._age[0]; } 
	}
	
	var johnCena = new Person(); 
	for(var i = 0; i < 100; i++) {
		johnCena.name = "john cena";
		johnCena.name = "john cena";
		johnCena.age = 22.0;
		johnCena.age = 22.0;
		johnCena.age = 22.0;
		johnCena.userId = i;
		johnCena.userId = i + 1;
		johnCena.userId = i + 2;
	}
}

function goodMath() {
	// Monomorphic use of operations
	function addInteger(x, y) {
		return x + y;
	}
	
	function addFloat(x, y) {
		return x + y;
	}
	
	function addString(x, y) {
		return x + y;
	}
	
	for(var i = 0; i < 100; i++) {
		addInteger(15346899, 236324512);
		addInteger(235176, 124351);
		addInteger(360450, 372033);
		addInteger(531802, 57400);
		addString("14234 sdfsfg sgh", "2 sdfgsdfg234");
		addString("2sdfsdf", "1sdfsdfsdf");
		addString("ewdftgsdftwer", "sfdgsdfgkiu sxc");
		addFloat(1.583324, 2.23456);
		addFloat(2236.06784, 1.0790567);
		addFloat(0.949397335833968, 0.0522764346833391);
	}
}

function goodLetDeclaration() {
	
	function Person(name, age) {
		this.name = name,
		this.age = age,
		this.bloodType = "",
		this.userId = 0,
		this.experience = 0.0;
	}
	
	function doDeclaration(x) {
		var johnCena = i,
			johnCena2 = "1",
			johnCena3 = 1.0,
			johnCena4 = new Date(),
			johnCena5 = 1 + i,
			realJohnCena = new Person("John Cena", 22),
			realJohnCena2 = new Person("John Cena", 22),
			realJohnCena3 = new Person("John Cena", 22);
		
		realJohnCena.bloodType = "AB",
		realJohnCena.userId = x,
		realJohnCena2.bloodType = "AB",
		realJohnCena2.experience = 1.5 + x,
		realJohnCena3.bloodType = "AB";
	}
	
	for(var i = 0; i < 50; i++) {
		doDeclaration(i);
	}
}

function goodArrayDeclaration() {
	// Array is not sparse, first value declared with [0] as well.
	function doDeclaration() {
		var johnCenas = new Array("And his name is John Cena!", "And his name is John Cena 2!");
	}
	
	for(var i = 0; i < 50; i++) {
		doDeclaration();
	}
}

console.log("\n\n");
console.log("\t-------------------------------");
console.log("\tWelcome to performance test!");
console.log("\t-------------------------------");
console.log("\n\n\n");

var iter = 10;

// Test 1
console.log("Testing for hidden classes: ");
console.log("This is a consistent result but not much of a major difference. \n");
console.time("b1");
for(var i = 0; i < iter; i++) {
	badPerson();
}
console.timeEnd("b1");

console.time("g1");
for(var i = 0; i < iter; i++) {
	goodPerson();
}
console.timeEnd("g1");
console.log("----------------------\n");

// Test 2
console.log("Testing for predictable data types: \n");
console.time("b2");
for(var i = 0; i < iter; i++) {
	badPerson2();
}
console.timeEnd("b2");

console.time("g2");
for(var i = 0; i < iter; i++) {
	goodPerson2();
}
console.timeEnd("g2");
console.log("----------------------\n");

// Test 3
console.log("Testing for data type changing: \n");
console.time("b3");
for(var i = 0; i < iter; i++) {
	badPerson3();
}
console.timeEnd("b3");

console.time("g3");
for(var i = 0; i < iter; i++) {
	goodPerson3();
}
console.timeEnd("g3");

/*
console.time("es2015-3");
for(var i = 0; i < iter; i++) {
	es2015Person3();
}
console.timeEnd("es2015-3");
*/

console.log("----------------------\n");

// Test 4
console.log("Testing for Monomorphic vs Polymorphic: \n");
console.time("b4");
for(var i = 0; i < iter; i++) {
	badMath();
}
console.timeEnd("b4");

console.time("g4");
for(var i = 0; i < iter; i++) {
	goodMath();
}
console.timeEnd("g4");
console.log("----------------------\n");

/* Requires Date *
// Test 5
console.log("Testing variable declaration: ");
console.log("This is an inconsistent result, there isn't any difference, please ignore. \n");
console.time("b5");
for(var i = 0; i < iter; i++) {
	badLetDeclaration();
}
console.timeEnd("b5");

console.time("g5");
for(var i = 0; i < iter; i++) {
	goodLetDeclaration();
}
console.timeEnd("g5");
console.log("----------------------\n");
*/

// Test 6
console.log("Testing array declaration: \n");
console.time("b5");
for(var i = 0; i < iter; i++) {
	badArrayDeclaration();
}
console.timeEnd("b5");

console.time("g5");
for(var i = 0; i < iter; i++) {
	goodArrayDeclaration();
}
console.timeEnd("g5");
console.log("----------------------\n");

console.log("\n\n");
