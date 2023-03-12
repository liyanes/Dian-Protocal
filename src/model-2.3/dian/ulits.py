import threading

class Local(dict):
    @staticmethod
    def _get_tid():
        return threading.get_ident()

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self._local = {}

    def __getitem__(self, key):
        return self._local[self._get_tid()][key]
    
    def __setitem__(self, key, value):
        if self._get_tid() not in self._local:
            self._local[self._get_tid()] = {}
        self._local[self._get_tid()][key] = value

    def __delitem__(self, key):
        del self._local[self._get_tid()][key]

    def __len__(self):
        if not self._get_tid() in self._local:
            return 0
        return len(self._local[self._get_tid()])
    
    def __iter__(self):
        if not self._get_tid() in self._local:
            return iter([])
        return iter(self._local[self._get_tid()])

    def __contains__(self, key):
        if not self._get_tid() in self._local:
            return False
        return key in self._local[self._get_tid()]
    
    def __repr__(self):
        if not self._get_tid() in self._local:
            return repr({})
        return repr(self._local[self._get_tid()])
    
    def __str__(self):
        if not self._get_tid() in self._local:
            return str({})
        return str(self._local[self._get_tid()])
    
    def __getattr__(self, name):
        if not self._get_tid() in self._local:
            return None
        return self._local[self._get_tid()][name]

    def __setattr__(self, name, value):
        self.__setitem__(name, value)

    def __delattr__(self, name):
        self.__delitem__(name)

    def values(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            return []
        return self._local[self._get_tid()].values(*args, **kwargs)
    
    def keys(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            return []
        return self._local[self._get_tid()].keys(*args, **kwargs)
    
    def items(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            return []
        return self._local[self._get_tid()].items(*args, **kwargs)
    
    def clear(self, *args, **kwargs):
        if self._get_tid() in self._local:
            self._local[self._get_tid()].clear(*args, **kwargs)

    def copy(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            return {}
        return self._local[self._get_tid()].copy(*args, **kwargs)
    
    def get(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            return None
        return self._local[self._get_tid()].get(*args, **kwargs)
    
    def pop(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            raise KeyError
        return self._local[self._get_tid()].pop(*args, **kwargs)
    
    def popitem(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            raise KeyError
        return self._local[self._get_tid()].popitem(*args, **kwargs)
    
    def setdefault(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            self._local[self._get_tid()] = {}
        return self._local[self._get_tid()].setdefault(*args, **kwargs)
    
    def update(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            self._local[self._get_tid()] = {}
        return self._local[self._get_tid()].update(*args, **kwargs)
    
    def fromkeys(self, *args, **kwargs):
        if not self._get_tid() in self._local:
            self._local[self._get_tid()] = {}
        return self._local[self._get_tid()].fromkeys(*args, **kwargs)
    
    def __eq__(self, other):
        if not self._get_tid() in self._local:
            return other == {}
        return self._local[self._get_tid()] == other
    
    def __ne__(self, other):
        if not self._get_tid() in self._local:
            return other != {}
        return self._local[self._get_tid()] != other
    
    def __lt__(self, other):
        if not self._get_tid() in self._local:
            return {} < other
        return self._local[self._get_tid()] < other
    
    def __le__(self, other):
        if not self._get_tid() in self._local:
            return {} <= other
        return self._local[self._get_tid()] <= other
    
    def __gt__(self, other):
        if not self._get_tid() in self._local:
            return {} > other
        return self._local[self._get_tid()] > other
    
    def __ge__(self, other):
        if not self._get_tid() in self._local:
            return {} >= other
        return self._local[self._get_tid()] >= other
    
    def __hash__(self):
        if not self._get_tid() in self._local:
            return hash({})
        return hash(self._local[self._get_tid()])
    
    def __bool__(self):
        if not self._get_tid() in self._local:
            return False
        return bool(self._local[self._get_tid()])
    
    def __dir__(self):
        if not self._get_tid() in self._local:
            return dir({})
        return dir(self._local[self._get_tid()])
    
    def __sizeof__(self):
        if not self._get_tid() in self._local:
            return 0
        import sys
        return sys.getsizeof(self._local[self._get_tid()])
    
    def __reduce__(self):
        if not self._get_tid() in self._local:
            return ({},)
        return self._local[self._get_tid()].__reduce__()
    
    def __reduce_ex__(self, protocol):
        if not self._get_tid() in self._local:
            return ({},)
        return self._local[self._get_tid()].__reduce_ex__(protocol)
    
    def __subclasshook__(self, C):
        if not self._get_tid() in self._local:
            return False
        return self._local[self._get_tid()].__subclasshook__(C)
    
    def __format__(self, format_spec):
        if not self._get_tid() in self._local:
            return format({}, format_spec)
        return format(self._local[self._get_tid()], format_spec)
    
    def __instancecheck__(self, instance):
        if not self._get_tid() in self._local:
            return False
        return self._local[self._get_tid()].__instancecheck__(instance)
    
    def __subclasscheck__(self, subclass):
        if not self._get_tid() in self._local:
            return False
        return self._local[self._get_tid()].__subclasscheck__(subclass)
    
    def threads(self):
        return [i.keys() for i in self._local if len(i) > 0]
    
    def clear_threads(self):
        for i in self._local:
            if self._local[i] == {}:
                del self._local[i]

    def iter_threads(self) :
        for i in self._local:
            yield i, self._local[i]

    def thread(self, tid):
        if tid in self._local:
            return self._local[tid]
        return []

    def change_thread(self, tid):
        if tid in self._local:
            self._local[self._get_tid()] = self._local[tid]
            del self._local[tid]
