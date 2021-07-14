#include <iostream>
#include <fstream>
#include <unordered_set>
#include <optional>
#include <stack>
#include <set>

// better with DAWG
struct Trie {
		std::optional<std::string> value{};
		std::array<Trie*, 26> children{};

		~Trie() {
			// recursive depth == length of words
			for (auto c : children)
				delete c;
		}
		bool empty() {
			return std::none_of(children.begin(),
													children.end(),
													[](auto ptr){ return ptr; });
		}
		bool contains(const char *word) {
			if (!word)
				return value.has_value();

			return children[word[0] - 'a']->contains(word + 1);
		}
};

struct LengthComp {
	bool operator()(auto lhs, auto rhs) const {
		return lhs.size() != rhs.size() ? lhs.size() < rhs.size() : lhs < rhs;
	}
};

class Dictionary {
		Trie dictionary{};
		size_t size_{0};

		void insert(const std::string& word) {
			if (std::any_of(word.begin(),
											word.end(),
											[](char c){ return !std::islower(c); }))
				return;

			Trie* node{&dictionary};
			for (auto c : word) {
				if (!node->children[c - 'a'])
					node->children[c - 'a'] = new Trie();

				// follow c edge
				node = node->children[c - 'a'];
			}
//			std::cout << word << " | ";
			node->value = word;
			++size_;
		}

	public:
		explicit Dictionary(const char* filename) {
			std::ifstream file;
			file.open(filename);
			std::string line;
			try {
				std::set<std::string, LengthComp> tmp{};
				while (file >> line) {
					if (line.size() > 3)
						tmp.insert(line);
				}
				file.close();
				for (auto& word : tmp) {
					insert(word);
				}
			}
			catch (const std::ifstream::failure& e) {
				std::cout << "Failed to open file";
			}
		}
		~Dictionary() = default;

		size_t size() {
			return size_;
		}

		std::string longest() {
			Trie* node{&dictionary};
			while (!node->empty()) {
				node = *std::find_if(node->children.begin(),
														 node->children.end(),
														 [](auto ptr){ return ptr; });
			}
			// no empty nodes without a value
			return *node->value;
		}
};

int main() {
	Dictionary dict{"/home/markus/data/build/biggest_word/dictionary/lawler.wordlist"};
	std::cout << dict.size() << '|';
	std::cout << dict.longest();
	return 0;
}
