#pragma once

/*
 * singleton definition example:
 * using LogS = Singleton<Log, CreateMeyers>;
 */

namespace NPP {
namespace Util {

	// This is how a Gamma Singleton would instantiate its object.
	template <class T> struct CreateGamma {
		static T* Create() {
			return new T;
		}
	};

	// This is how a Meyers Singleton would instantiate its object.
	template <class T> struct CreateMeyers {
		static T* Create() {
			static T _instance;
			return &_instance;
		}
	};

	// This Singleton class accepts different creation policies
	template <class T, template<class> class CreationPolicy=CreateMeyers>
		class Singleton
		{
			public:
				static T& Instance() {
					if (!m_pInstance)
						m_pInstance=CreationPolicy<T>::Create();
					return *m_pInstance;
				}

			private:
				Singleton();
				~Singleton();
				Singleton(Singleton const&);
				Singleton& operator=(Singleton const&);

				static T* m_pInstance;
		};

	template <class T, template<class> class C>
		T* Singleton<T,C>::m_pInstance = 0;

} // namespace Util
} // namespace NPP
