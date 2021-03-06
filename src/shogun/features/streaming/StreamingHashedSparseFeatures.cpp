/*
 * This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Authors: Evangelos Anagnostopoulos, Heiko Strathmann, Bjoern Esser,
 *          Sergey Lisitsyn, Viktor Gal
 */

#include <shogun/features/streaming/StreamingHashedSparseFeatures.h>
#include <shogun/features/hashed/HashedSparseFeatures.h>
#include <shogun/io/streaming/StreamingFileFromSparseFeatures.h>

namespace shogun
{

template <class ST>
StreamingHashedSparseFeatures<ST>::StreamingHashedSparseFeatures()
{
	init(NULL, false, 0, 0, false, true);
}

template <class ST>
StreamingHashedSparseFeatures<ST>::StreamingHashedSparseFeatures(const std::shared_ptr<StreamingFile>& file,
	bool is_labelled, int32_t size, int32_t d, bool use_quadr, bool keep_lin_terms)
{
	init(file, is_labelled, size, d, use_quadr, keep_lin_terms);
}

template <class ST>
StreamingHashedSparseFeatures<ST>::StreamingHashedSparseFeatures(std::shared_ptr<SparseFeatures<ST>> dot_features,
	int32_t d, bool use_quadr, bool keep_lin_terms, float64_t* lab)
{
	ASSERT(dot_features);

	auto file = std::make_shared<StreamingFileFromSparseFeatures<ST>>(dot_features, lab);
	bool is_labelled = (lab != NULL);
	int32_t size = 1024;

	init(file, is_labelled, size, d, use_quadr, keep_lin_terms);

	parser.set_free_vectors_on_destruct(false);
	seekable=true;
}

template <class ST>
StreamingHashedSparseFeatures<ST>::~StreamingHashedSparseFeatures()
{
}

template <class ST>
void StreamingHashedSparseFeatures<ST>::init(std::shared_ptr<StreamingFile> file, bool is_labelled,
	int32_t size, int32_t d, bool use_quadr, bool keep_lin_terms)
{
	dim = d;
	SG_ADD(&dim, "dim", "Size of target dimension");

	use_quadratic = use_quadr;
	keep_linear_terms = keep_lin_terms;

	SG_ADD(&use_quadratic, "use_quadratic", "Whether to use quadratic features");
	SG_ADD(&keep_linear_terms, "keep_linear_terms", "Whether to keep the linear terms or not");

	has_labels = is_labelled;
	if (file)
	{
		working_file = file;

		parser.init(file, is_labelled, size);
		seekable = false;
	}
	else
		file = NULL;

	set_read_functions();
	parser.set_free_vector_after_release(false);

	set_generic<ST>();
}

template <class ST>
float32_t StreamingHashedSparseFeatures<ST>::dot(std::shared_ptr<StreamingDotFeatures> df)
{
	ASSERT(df);
	ASSERT(df->get_feature_type() == get_feature_type())
	ASSERT(strcmp(df->get_name(),get_name())==0)

	auto hdf = std::static_pointer_cast<StreamingHashedSparseFeatures<ST>>(df);
	return current_vector.sparse_dot(hdf->current_vector);
}

template <class ST>
float32_t StreamingHashedSparseFeatures<ST>::dense_dot(const float32_t* vec2, int32_t vec2_len)
{
	ASSERT(vec2_len == dim);

	float32_t result = 0;
	for (index_t i=0; i<current_vector.num_feat_entries; i++)
		result += vec2[current_vector.features[i].feat_index] * current_vector.features[i].entry;

	return result;
}

template <class ST>
void StreamingHashedSparseFeatures<ST>::add_to_dense_vec(float32_t alpha, float32_t* vec2,
	int32_t vec2_len, bool abs_val)
{
	ASSERT(vec2_len == dim);

	if (abs_val)
		alpha = Math::abs(alpha);

	for (index_t i=0; i<current_vector.num_feat_entries; i++)
		vec2[current_vector.features[i].feat_index] += alpha * current_vector.features[i].entry;
}

template <class ST>
int32_t StreamingHashedSparseFeatures<ST>::get_dim_feature_space() const
{
	return dim;
}

template <class ST>
const char* StreamingHashedSparseFeatures<ST>::get_name() const
{
	return "StreamingHashedSparseFeatures";
}

template <class ST>
int32_t StreamingHashedSparseFeatures<ST>::get_num_vectors() const
{
	return 1;
}

template <class ST>
void StreamingHashedSparseFeatures<ST>::set_vector_reader()
{
	SG_DEBUG("called inside set_vector_reader");
	parser.set_read_vector(&StreamingFile::get_sparse_vector);
}

template <class ST>
void StreamingHashedSparseFeatures<ST>::set_vector_and_label_reader()
{
	parser.set_read_vector_and_label(&StreamingFile::get_sparse_vector_and_label);
}

template <class ST>
EFeatureType StreamingHashedSparseFeatures<ST>::get_feature_type() const
{
	return F_UINT;
}

template <class ST>
EFeatureClass StreamingHashedSparseFeatures<ST>::get_feature_class() const
{
	return C_STREAMING_SPARSE;
}

template <class ST>
void StreamingHashedSparseFeatures<ST>::start_parser()
{
	if (!parser.is_running())
		parser.start_parser();
}

template <class ST>
void StreamingHashedSparseFeatures<ST>::end_parser()
{
	parser.end_parser();
}

template <class ST>
float64_t StreamingHashedSparseFeatures<ST>::get_label()
{
	return current_label;
}

template <class ST>
bool StreamingHashedSparseFeatures<ST>::get_next_example()
{
	SGSparseVector<ST> tmp;
	if (parser.get_next_example(tmp.features,
		tmp.num_feat_entries, current_label))
	{
		current_vector = HashedSparseFeatures<ST>::hash_vector(tmp, dim,
				use_quadratic, keep_linear_terms);
		tmp.features = NULL;
		tmp.num_feat_entries = -1;
		return true;
	}
	return false;
}

template <class ST>
void StreamingHashedSparseFeatures<ST>::release_example()
{
	parser.finalize_example();
}

template <class ST>
int32_t StreamingHashedSparseFeatures<ST>::get_num_features()
{
	return dim;
}

template <class ST>
SGSparseVector<ST> StreamingHashedSparseFeatures<ST>::get_vector()
{
	return current_vector;
}

template class StreamingHashedSparseFeatures<bool>;
template class StreamingHashedSparseFeatures<char>;
template class StreamingHashedSparseFeatures<int8_t>;
template class StreamingHashedSparseFeatures<uint8_t>;
template class StreamingHashedSparseFeatures<int16_t>;
template class StreamingHashedSparseFeatures<uint16_t>;
template class StreamingHashedSparseFeatures<int32_t>;
template class StreamingHashedSparseFeatures<uint32_t>;
template class StreamingHashedSparseFeatures<int64_t>;
template class StreamingHashedSparseFeatures<uint64_t>;
template class StreamingHashedSparseFeatures<float32_t>;
template class StreamingHashedSparseFeatures<float64_t>;
template class StreamingHashedSparseFeatures<floatmax_t>;
}
