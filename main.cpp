#include <iostream>
#include <fstream>

#include <unordered_set>
#include <set>
#include <vector>

#include <curl/curl.h>
#include <cstring>
#include <optional>

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
				for (auto word_ptr : words)
					std::cout << *word_ptr << " | ";
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
			curl_easy_setopt(crl, CURLOPT_ACCEPT_ENCODING, "");
			curl_easy_setopt(crl, CURLOPT_FOLLOWLOCATION, 1);

			FILE* c_file{fopen(path, "w")};
			curl_easy_setopt(crl, CURLOPT_WRITEDATA, c_file);

			curl_easy_perform(crl);
			curl_easy_cleanup(crl);

			fclose(c_file);
		}

	public:
		explicit Dictionary(const std::string& path, const std::optional<std::string>& url) {
			if (url.has_value()) {
				std::cout << "Downloading...\n";
				download(path.c_str(), url.value().c_str());
			}

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
				if (tmp.empty()) {
					std::cout << "No words found in file\n";
					return;
				}
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

struct CmdLineParser {
		// OPTIONS
		std::string wordlist_path{};
		std::optional<std::string> url{};
		int min_size{6};

		CmdLineParser(int argc, const char *argv[]) {
			// first argument is always wordlist
			if (argc == 1)
				throw InsufficientArgs();

			wordlist_path = argv[1];
			std::cout << "Parsed wordlist path: " << argv[1] << '\n';
			for (int i{2}; i + 1 < argc; i += 2) {
				if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--url") == 0) {
					url = argv[i + 1];
					std::cout << "Parsed url: " << argv[i + 1] << '\n';
				}
				else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--min_size") == 0) {
					min_size = std::stoi(argv[i + 1]);
					std::cout << "Parsed min_size: " << argv[i + 1] << '\n';
				}
				else
					throw UnknownFlag(argv[i]);
			}
		}

		template <class ErrorType>
		struct CmdLineException : public std::exception {
			std::string error{ErrorType::message};

			CmdLineException() = default;
			explicit CmdLineException(const char* err) {
				error.append(err);
			}

			[[nodiscard]] const char* what() const noexcept override {
				return error.c_str();
			}
		};

		struct UnknownFlagMsg {
			static constexpr char message[]{"Unknown command line flag: "};
		};
		using UnknownFlag = CmdLineException<UnknownFlagMsg>;

		struct InsufficientArgsMsg {
			static constexpr char message[]{"Insufficient arguments passed to the command line"};
		};
		using InsufficientArgs = CmdLineException<InsufficientArgsMsg>;
};

int main(int argc, const char * argv[]) {
	CmdLineParser args{argc, argv};
	Dictionary dict{args.wordlist_path, args.url};
	dict.print();
	return 0;
}
