use crate::intern::InternedStringTable;

pub trait ASTPrettyPrint {
    fn pretty_print(&self, indent: usize) -> String;
}

pub trait ASTPrettyPrintInterned {
    fn pretty_print_interned(&self, indent: usize, table: &InternedStringTable) -> String;
}

#[derive(Debug)]
pub struct ASTNodeNumber {
    value: u64,
}

impl ASTNodeNumber {
    pub fn new(value: u64) -> Self {
        ASTNodeNumber { value }
    }
}

impl ASTPrettyPrint for ASTNodeNumber {
    fn pretty_print(&self, _indent: usize) -> String {
        self.value.to_string()
    }
}

#[derive(Debug)]
pub struct ASTNodeString {
    value: String,
}

impl ASTNodeString {
    pub fn new(value: String) -> Self {
        ASTNodeString { value }
    }
}

impl ASTPrettyPrint for ASTNodeString {
    // TODO: handle pretty print with escaping for quotes
    fn pretty_print(&self, _indent: usize) -> String {
        format!("\"{}\"", self.value)
    }
}

#[derive(Debug)]
pub struct ASTNodeSymbol {
    value: usize,
    is_keyword: bool,
}

impl ASTNodeSymbol {
    pub fn new(value: String, table: &mut InternedStringTable) -> Self {
        let is_keyword = value.starts_with(":");
        let value = table.intern(value);
        ASTNodeSymbol { value, is_keyword }
    }

    pub fn is_keyword(&self) -> bool {
        self.is_keyword
    }

    pub fn value<'a>(&self, table: &'a InternedStringTable) -> Option<&'a String> {
        table.get(self.value)
    }
}

impl ASTPrettyPrintInterned for ASTNodeSymbol {
    fn pretty_print_interned(&self, _indent: usize, table: &InternedStringTable) -> String {
        if let Some(value) = table.get(self.value) {
            value.clone()
        } else {
            "<not-found-interned>".to_string()
        }
    }
}

#[derive(Debug)]
pub struct ASTNodeList {
    elements: Vec<ASTNode>,
}

impl ASTNodeList {
    pub fn from_elements(elements: Vec<ASTNode>) -> Self {
        ASTNodeList { elements }
    }
}

impl ASTNodeList {
    fn pretty_print(
        &self,
        indent: usize,
        kw_table: &InternedStringTable,
        sym_table: &InternedStringTable,
    ) -> String {
        let mut result = String::new();
        result.push_str(&" ".repeat(indent));
        result.push('(');

        for (i, element) in self.elements.iter().enumerate() {
            result.push_str(&element.pretty_print(0, kw_table, sym_table));
            if i < self.elements.len() - 1 {
                result.push(' ');
            }
        }

        result.push(')');
        result
    }
}

#[derive(Debug)]
pub enum ASTNode {
    Number(ASTNodeNumber),
    String(ASTNodeString),
    Symbol(ASTNodeSymbol),
    List(ASTNodeList),
    Nil,
    True,
}

impl ASTNode {
    pub fn pretty_print(
        &self,
        indent: usize,
        kw_table: &InternedStringTable,
        sym_table: &InternedStringTable,
    ) -> String {
        match self {
            ASTNode::Number(n) => n.pretty_print(indent),
            ASTNode::String(s) => s.pretty_print(indent),
            ASTNode::Symbol(s) => {
                let table = if s.is_keyword() { kw_table } else { sym_table };
                s.pretty_print_interned(indent, table)
            }
            ASTNode::List(l) => l.pretty_print(indent, kw_table, sym_table),
            ASTNode::Nil => "nil".to_string(),
            ASTNode::True => "t".to_string(),
        }
    }
}
