def my_generator2():
    for i in range(5):
        yield i
        
def my_generator():
    yield 1
    yield 2

gen = my_generator()
print(gen.__next__())
print(gen.send(2))
print(gen.send(3))