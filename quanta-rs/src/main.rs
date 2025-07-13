use std::collections::VecDeque;
use std::io::{self, Read, stdout};

mod ast;
mod intern;

use ast::ASTPrettyPrint;

fn is_terminator(c: char) -> bool {
    c.is_whitespace() || c == '(' || c == ')' || c == '"' || c == ':' || c == ';'
}

struct Parser {
    input: VecDeque<char>,
    program: Vec<ast::ASTNode>,
    symbol_table: intern::InternedStringTable,
    keyword_table: intern::InternedStringTable,
}

impl Parser {
    fn new(input: String) -> Self {
        Parser {
            input: input.chars().collect(),
            program: Vec::new(),
            symbol_table: intern::InternedStringTable::new(),
            keyword_table: intern::InternedStringTable::new(),
        }
    }

    pub fn parse(&mut self) -> Result<(), String> {
        while !self.input.is_empty() {
            match self.read_atom() {
                Ok(node) => self.program.push(node),
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }

    pub fn program(&self) -> &Vec<ast::ASTNode> {
        &self.program
    }

    pub fn pretty_print(&self) -> String {
        self.program
            .iter()
            .map(|node| node.pretty_print(0, &self.keyword_table, &self.symbol_table))
            .collect::<Vec<String>>()
            .join("\n")
    }

    fn read_form(&mut self) -> Result<ast::ASTNode, String> {
        let mut elements = Vec::new();

        let mut terminated = false;

        while let Some(&c) = self.input.front() {
            if c.is_whitespace() {
                self.input.pop_front();
            } else if c == ')' {
                terminated = true;
                self.input.pop_front();
                break;
            } else {
                match self.read_atom() {
                    Ok(node) => elements.push(node),
                    Err(e) => return Err(e),
                }
            }
        }

        if terminated {
            Ok(ast::ASTNode::List(ast::ASTNodeList::from_elements(
                elements,
            )))
        } else {
            Err("Unterminated list".to_string())
        }
    }

    fn read_atom(&mut self) -> Result<ast::ASTNode, String> {
        if self.input.is_empty() {
            return Err("Unexpected end of input".to_string());
        }

        while let Some(&c) = self.input.front() {
            if c.is_whitespace() {
                self.input.pop_front();
            } else {
                break;
            }
        }

        let next = self.input.pop_front();
        if next.is_none() {
            return Err("Unexpected end of input".to_string());
        }

        let c = next.unwrap();

        match c {
            '(' => self.read_form(),

            '0'..='9' => {
                // parse a number
                let mut number_str = String::new();
                number_str.push(c);

                while let Some(&next) = self.input.front() {
                    if next.is_digit(10) {
                        number_str.push(self.input.pop_front().unwrap());
                    } else {
                        break;
                    }
                }

                match number_str.parse::<u64>() {
                    Ok(number_value) => {
                        Ok(ast::ASTNode::Number(ast::ASTNodeNumber::new(number_value)))
                    }
                    Err(_) => Err(format!("Invalid number: {}", number_str)),
                }
            }

            '"' => {
                let mut string_value = String::new();

                let mut terminated = false;

                while let Some(next) = self.input.pop_front() {
                    match next {
                        '"' => {
                            terminated = true;
                            break;
                        }

                        // TODO: escaping
                        other => {
                            string_value.push(other);
                        }
                    }
                }

                if !terminated {
                    return Err("Unterminated string literal".to_string());
                } else if let Some(&next) = self.input.front() {
                    if !is_terminator(next) {
                        return Err(format!(
                            "Unexpected character after string literal: {}",
                            next
                        ));
                    }
                }

                Ok(ast::ASTNode::String(ast::ASTNodeString::new(string_value)))
            }

            other => {
                let mut symbol_value = String::new();

                symbol_value.push(other);

                while let Some(&next) = self.input.front() {
                    if !is_terminator(next) {
                        symbol_value.push(self.input.pop_front().unwrap());
                    } else {
                        break;
                    }
                }

                if symbol_value.is_empty() {
                    Err(format!("Unexpected character: {}", other))
                } else {
                    let table = if symbol_value.starts_with(":") {
                        &mut self.keyword_table
                    } else {
                        &mut self.symbol_table
                    };
                    Ok(match symbol_value.as_str() {
                        "nil" => ast::ASTNode::Nil,
                        "t" => ast::ASTNode::True,
                        _ => ast::ASTNode::Symbol(ast::ASTNodeSymbol::new(symbol_value, table)),
                    })
                }
            }
        }
    }
}

fn main() {
    let mut buffer = String::new();

    match io::stdin().read_to_string(&mut buffer) {
        Ok(_) => (),
        Err(e) => {
            eprintln!("Error reading from stdin: {}", e);
            return;
        }
    }

    let mut parser = Parser::new(buffer);

    parser.parse().unwrap_or_else(|e| {
        eprintln!("Error parsing input: {}", e);
        std::process::exit(1);
    });

    print!("{}\n", parser.pretty_print());
}
