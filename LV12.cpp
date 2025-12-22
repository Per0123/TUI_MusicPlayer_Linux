#include <iostream>
#include <vector>
using namespace std;

static void zad_a() {
	int n1, n2; cin >> n1 >> n2;

	for (int i = 0; i < n2; ++i) {
		cout << n1+i << ". godina\n";

		for (int j = 0; j < 12; ++j) {
			cout << i+1 << ". mjesec\n";
		}
	}
}

static void zad_b() {
	vector<int> li;

	for (int i = 0; i < 3; ++i) {
		int a; cin >> a;
		li.push_back(a);
	}

	for (int j = 0; j < 3; ++j) {
		cout << "Visekratnici broja " << li[j] << " su: ";
		for (int i = 0; i < 6; ++i) {
			cout << li[j] * (i + 1);

			if (i == 5) {
				cout << ". " << endl;
			}
			else {
				cout << ", ";
			}
		}
	}
}

static void zad_c() {
	vector<int> lx, ly; bool bo = false;
	for (int i = 0; i < 3; ++i) {
		int x, y; cin >> x >> y;

		lx.push_back(x-1);
		ly.push_back(y-1);
	}

	for (int i = 0; i < 15; ++i) {
		for (int j = 0; j < 15; ++j) {
			for (int k = 0; k < 3; ++k) {
				if (i == lx[k] and j == ly[k]) {
					bo = true;
				}
			}

			if (bo) {
				cout << "* ";
			}
			else {
				cout << ". ";
			}

			bo = false;
		}
		cout << endl;
	}
}

static void zad_d() {
	int n; cin >> n;

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (i == j) {
				cout << "1 "; 
			}
			else {
				cout << "0 "; 
			}
		}
		cout << endl;
	}

}

static void zad_e() {
	int n, m;
	char ch1, ch2;

	do {
		cin >> n >> m >> ch1 >> ch2;
	} while ((n < 3 || m < 3) || (n % 2 == 0 || m % 2 == 0));

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			if (i == 0 || i == n - 1 || j == 0 || j == m - 1) {
				cout << ch1 << " ";
			}
			else if ((i % 2 == 0 && j % 2 != 0) || (i % 2 != 0 && j % 2 == 0)) {
				cout << ch2 << " ";
			}
			else {
				cout << "  "; 
			}
		}
		cout << endl;
	}
	// !!!
}

int main() {
	int zad; cin >> zad;

	switch (zad) {
	case 1:
		zad_a();
		break;
	case 2:
		zad_b();
		break;
	case 3:
		zad_c();
		break;
	case 4:
		zad_d();
		break;
	case 5:
		zad_e();
		break;
	}
}
