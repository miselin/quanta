use std::collections::HashMap;

#[derive(Debug)]
pub struct InternedStringTable {
    strings: HashMap<String, usize>,
    interned: Vec<String>,
}

impl InternedStringTable {
    pub fn new() -> Self {
        InternedStringTable {
            strings: HashMap::new(),
            interned: Vec::new(),
        }
    }

    pub fn intern(&mut self, value: String) -> usize {
        if let Some(&index) = self.strings.get(&value) {
            index
        } else {
            let index = self.interned.len();
            self.strings.insert(value.clone(), index);
            self.interned.push(value);
            index
        }
    }

    pub fn get(&self, index: usize) -> Option<&String> {
        self.interned.get(index)
    }
}
