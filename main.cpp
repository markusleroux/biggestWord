#include <iostream>
#include <fstream>
#include <unordered_set>
#include <optional>
#include <stack>
#include <set>
#include <vector>
#include <ranges>

#include <curl/curl.h>
#include <algorithm>

struct LengthComp {
	bool operator()(auto v, auto w) const {
		return v.size() != w.size() ? v.size() < w.size() : v < w;
	}
};

void set_intersection(std::set<std::string*>& acc,
											const std::set<std::string*>& val) {
	if (acc.empty() || val.empty()) {
		acc.clear();
		return;
	}

	auto acc_iter{acc.begin()};
	for (auto val_iter{val.begin()};
			acc_iter != acc.end() && val_iter != val.end(); ) {
		if (*acc_iter < *val_iter)
			acc_iter = acc.erase(acc_iter);
		else if (*val_iter < *acc_iter)
			++val_iter;
		else {
			++acc_iter;
			++val_iter;
		}
	}

	acc.erase(acc_iter, acc.end());
}

inline std::array<int, 26> count(const std::string& word) {
	std::array<int, 26> arr{};
	for (auto c : word)
		++arr[c - 'a'];
	return arr;
}

// blank is getting into counts somehow
class Dictionary {
		struct Repr {
			std::vector<std::string*> words{};
			std::array<std::vector<std::set<std::string*>>, 26> counts{{{}}};

			~Repr() {
				for (auto ptr : words)
					delete ptr;
			}

			void printWords(int min_size = 6) {
				std::cout << "\nWORDS: \n";
				std::ranges::for_each(words,
															[min_size](const std::string* word_ptr){
																	if (word_ptr->size() >= min_size)
																		std::cout << *word_ptr << " | ";
															});
			}


		private:
			[[maybe_unused]] void print_count() {
				for (int i{}; i < counts.size(); ++i) {
					if (counts[i].empty())
						continue;
					std::cout << "Character: " << i;
					std::cout << "\n------------------------------\n";
					for (int j{}; j < counts[i].size(); ++j) {
						std::cout << "Level: " << j << " - ";
						for (const auto* string_ptr : counts[i][j]) {
							if (not string_ptr)
								throw;
							std::cout << *string_ptr << " | ";
						}
						std::cout << '\n';
					}
					std::cout << "------------------------------\n";
				}
			}
		public:


			void insert(const std::string& word, const std::array<int, 26> count) {
				words.emplace_back(new std::string{word});
				for (int i{}; i < 26; ++i) {
					while (counts[i].size() <= count[i])
						counts[i].emplace_back();
					counts[i][count[i]].insert(words.back());
				}
			}

			bool invariant() {
				for (const auto& v : counts) {
					int res{};
					for (const auto& s : v)
						res += s.size();
					if (res != words.size())
						return false;
				}
				return true;
			}

			std::set<std::string*> bound(const std::array<int, 26>& word_count) {
				std::set<std::string*> res{};
				for (const auto& w : words)
					res.emplace(w);

				for (int c{}; !res.empty() && c < 26; ++c) {
					auto val{fewer_than_n_char(c, word_count[c])};
					set_intersection(res, val);
				}
				return res;
			}

			std::set<std::string*> fewer_than_n_char(int c, int n) {
				std::set<std::string*> result{};
				n = std::min(int(counts[c].size()) - 1, n);
				while (n >= 0) {
					result.insert(counts[c][n].begin(), counts[c][n].end());
					--n;
				}
				return result;
			}
		} repr_;
		int last_length{1};

		void insert(const std::string& word) {
			if (word.size() == 0 ||
					std::any_of(word.begin(),
											word.end(),
											[](char c){ return !std::islower(c); }))
				return;
			if (last_length != word.size()) {
				last_length = int(word.size());
				std::cout << last_length << " | ";
				std::cout.flush();
			}

			std::array<int, 26> w_count{count(word)};
			if (repr_.bound(w_count).empty())
				repr_.insert(word, w_count);
		}

		void download(const char* path, const char* url) {
			CURL* crl{curl_easy_init()};

			curl_easy_setopt(crl, CURLOPT_URL, url);
			curl_easy_setopt(crl, CURLOPT_ACCEPT_ENCODING, "deflate");

			FILE* c_file{fopen(path, "w")};
			curl_easy_setopt(crl, CURLOPT_WRITEDATA, c_file);

			curl_easy_perform(crl);
			curl_easy_cleanup(crl);

			fclose(c_file);
		}

	public:
		explicit Dictionary(const char* path, const char* url = nullptr) {
			if (url)
				download(path, url);

			std::ifstream file;
			try {
				file.open(path);
				std::string line;

				std::set<std::string, LengthComp> tmp{};
				while (file >> line) {
					if (line.size() >= 3)
						tmp.insert(line);
				}
				file.close();
				for (auto& word : tmp) {
					if (not repr_.invariant())
						throw;
					insert(word);
				}
			}
			catch (const std::ifstream::failure& e) {
				std::cout << "Failed to open file";
			}
		}
		~Dictionary() = default;

		void print(int n = 6) {
			repr_.printWords(n);
		}

		size_t size() {
			return repr_.words.size();
		}
};

int main() {
	Dictionary dict{"/home/markus/data/build/biggest_word/dictionary/test.wordlist",
									"https://raw.githubusercontent.com/dolph/dictionary/master/popular.txt"};
	dict.print();
	return 0;
}
